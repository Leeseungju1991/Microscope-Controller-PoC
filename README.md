# WLI 현미경 모터 컨트롤러 펌웨어 (STM32G474) · PoC

> **제작자 / Author — LEE SEUNG JU**
>
> ㈜파크시스템스 재직 중 설계한 WLI Controller 펌웨어 아키텍처(모터 제어·PID·CRC16 통신 프로토콜)를
> 동일한 핵심 로직으로 재구현해 공개한 포트폴리오용 저장소입니다. 실 납품 제품의 고객사·내부 코드는
> 포함하지 않으며, 아키텍처와 알고리즘을 그대로 재현했습니다.

WLI(White-Light Interferometry) 현미경의 스테이지/스캐너 구동을 위한 모터 컨트롤러 펌웨어입니다.
RS232 바이너리 프로토콜로 PC와 통신하며, 2중 PID + 전류제어, 가감속 모션 프로파일,
인코더 피드백, 마이크로스텝 구동, 카메라 One-Pulse 스냅샷 트리거, 보호회로(과전류/과온/타임아웃)를
1kHz 결정론적 스케줄러 위에서 처리합니다. STM32G474 CubeIDE 타깃과 PC 시뮬레이터(POSIX) 양쪽을
동일 코드로 빌드/검증할 수 있습니다.

---

## 1. 구성 흐름도

### 1-1. 데이터 흐름 (PC ↔ FW ↔ 하드웨어)

```
                ┌──────────────────────────── PC (Host) ───────────────────────────┐
                │  tools/pc_send_cmd.py  (프로토콜 규격 테스트 / pyserial 송신)        │
                └───────────────┬──────────────────────────▲────────────────────────┘
                      RS232 TX   │  AA55│LEN│CMD│PAYLOAD│CRC16  │ Telemetry(JSON, 10Hz)
                                 ▼                              │
   ┌─────────────────────────── STM32G474 Firmware ─────────────────────────────────┐
   │                                                                                 │
   │  uart_drv ─► cmd_parser(프레임/CRC) ─► command dispatch ─► system_ctx_t(목표값)  │
   │                                                                  │              │
   │   adc_drv(DMA) ─► sensor(평균/환산) ─► current_mA / temp_C / vref │              │
   │                                                                  ▼              │
   │   encoder_drv ─► encoder(위치/속도) ──────────────►  ┌──────── control ───────┐  │
   │                                                      │ Position PID (outer)   │  │
   │                          motion_profile(가감속) ───► │  → Speed PID (inner)   │  │
   │                                                      │   → Current PI(전류원) │  │
   │                                                      └──────────┬─────────────┘  │
   │   protection(과전류/과온/오버스피드/staleness/타임아웃) ◄────────┤ pwm_duty        │
   │                                                                 ▼               │
   │   fsm(IDLE/RUN/FAULT/HOLD/CAL) ─► motor_drv(PWM/마이크로스텝/EN) ─► 모터          │
   │   camera(One-Pulse) ─► camera_trigger_drv ─► 카메라 스냅샷 트리거                 │
   │   telemetry ─► uart_drv(TX) ──────────────────────────────────────────────────► │
   └─────────────────────────────────────────────────────────────────────────────────┘
```

### 1-2. 스케줄링 흐름 (`scheduler` 1ms 틱 기반)

```
 timer_drv 1ms tick
      │
      ├─ 1kHz  ▶ sensor_process → encoder_update → control(3중 루프) → protection_fast → motor PWM
      ├─ 100Hz ▶ motion_profile step → protection_slow(타임아웃/staleness) → fsm 천이
      └─ 10Hz  ▶ telemetry 프레임 송신(TX)
 (ISR) ADC DMA half/full ▶ sensor_on_adc_dma_block (블록 평균)
 (ISR) UART RX byte       ▶ cmd_parser 바이트 단위 피드 → 프레임 완성 시 dispatch
```

### 1-3. 계층 구조 (IA)

```
app/      FSM · Scheduler · Control(3-loop) · MotionProfile · dual PID   (제어 로직)
service/  cmd_parser · crc16 · sensor · temp_monitor · encoder · camera · protection · telemetry
driver/   uart · adc(DMA) · motor(PWM) · encoder · timer · camera_trigger (HW 추상 API)
hal/      hal_if (HAL/LL 래핑)
platform/ platform_stm32g474_cubeide.c (실타깃) · platform_posix.c (PC 시뮬레이터)
include/  common.h · config.h(튜닝/임계값) · types.h(system_ctx_t)
tools/    pc_send_cmd.py (프로토콜 프레임 생성/송신 테스트)
```

---

## 2. 동작 설명 (5줄)

1. **RS232 바이너리 프로토콜**: `AA55 | LEN | CMD | PAYLOAD | CRC16(CCITT)` 프레임을 바이트 단위로 파싱하고 CRC로 무결성을 검증한다.
2. **3중 제어 루프 + 전류제어**: 위치 PID → 속도 PID → 전류 PI(전류원) 캐스케이드로 모터를 정밀 구동한다.
3. **가감속 모션 프로파일 + 인코더 피드백**: accel/decel 제한 setpoint를 생성하고 TIM 엔코더 모드로 위치/속도를 추정한다.
4. **카메라 One-Pulse 스냅샷**: TIM One-Pulse(delay+width)로 정확한 단일 트리거 펄스를 발생시켜 촬영을 동기화한다.
5. **보호/상태관리/텔레메트리**: 과전류·과온·오버스피드·센서 staleness·UART 타임아웃을 감시하고, FSM 상태와 측정값을 10Hz JSON으로 송신한다.

---

## 3. 핵심 알고리즘

### 1. RS232 통신 프로토콜 (`service/cmd_parser`, `service/crc16`, `driver/uart_drv`)
- **프레임 포맷**: `SOF(0xAA 0x55)` → `LEN(1B)` → `CMD(1B)` → `PAYLOAD(N)` → `CRC16-CCITT(LE, 2B)`.
- **바이트 스트림 파서**: 상태머신으로 SOF 동기화 → 길이 수신 → 페이로드 누적 → CRC 검증.
- **명령 디스패치**: `0x01 start` / `0x02 stop` / `0x04 get_status` / `0x05 clear_fault` 등 즉시 명령.
- **통신 타임아웃**: `RUN` 상태에서 명령 미수신이 `PROT_UART_TIMEOUT_MS` 초과 시 안전 정지.

### 2. 전류제어 (Current Source) (`app/control`, `app/pid`, `service/sensor`)
- ADC 전류 채널 raw → mA 환산(DMA 블록 평균으로 노이즈 저감) → Current PI 루프가 PWM duty 산출(전류원 동작).
- 명령 인터페이스: `0x0A set_current(mA)`, `0x0B set_pid_current(Kp,Ki)`. `CURRENT_SP_MIN/MAX_mA` 클램프 + `PROT_OVERCURRENT_mA` 보호.

### 3. 가감속 모션 프로파일 (`app/motion_profile`)
- `MAX_SPEED_RPM` 상한 클램프, `MAX_ACCEL_RPM_S` 기반 매 스텝 setpoint 변화율 제한(트라페조이달 프로파일).

### 4. 인코더 피드백 (`service/encoder`, `driver/encoder_drv`)
- TIM Encoder 모드 카운트 → `position_rev`(CPR 기준), 카운트 차분/Δt → `speed_rpm`.

### 5. 마이크로스텝 구동 (`driver/motor_drv`, `app/control`)
- `MOTOR_PWM_MIN/MAX_DUTY` 범위의 듀티, 스텝 분해능 설정에 따른 마이크로스텝 시퀀스, FSM/보호 조건에 따른 출력단 enable/disable.

### 6. 카메라 One-Pulse 스냅샷 (`service/camera`, `driver/camera_trigger_drv`)
- TIM One-Pulse 모드로 `delay_us` 후 `width_us` 폭의 단일 펄스를 발생시켜 스테이지 위치/모션 이벤트와 촬영을 동기화.

### 7. 2중 PID (위치/속도) (`app/pid`, `app/control`)
- Position PID(Outer) → 속도 setpoint → Speed PID(Inner) → 전류 setpoint 캐스케이드. `0x06 set_pid_dual`로 게인 설정(Q8.8 고정소수), 출력 클램프+안티와인드업.

### 8. 보호회로 & 상태관리 (`service/protection`, `app/fsm`)
- 고속 보호(1kHz): 과전류/과온/오버스피드 즉시 차단. 저속 보호(100Hz): 센서 staleness, UART 타임아웃.
- FSM: `IDLE / RUN / FAULT / HOLD / CAL` 상태 천이, `clear_fault`로 복구.

### 9. 온도 모니터링 (`service/temp_monitor`, `service/sensor`)
- 내부 TempSensor + VREFINT ADC raw → 보정 전압 → °C 환산. `PROT_OVERTEMP_C` 초과 시 보호회로로 즉시 정지.

### 10. 텔레메트리 (`service/telemetry`)
- 10Hz 주기 송신: 상태/결함/위치/속도/전류/온도/카운터를 JSON 페이로드(`st, flt, pos, tpos, rpm, trpm, ma, tma, tc, d, cnt`)로 프레임화.

---

## 4. 빌드 & 검증

### PC 시뮬레이터 (POSIX)
```bash
cmake -S . -B build && cmake --build build
./build/stm32g474_fw_sim          # 1kHz 스케줄러 동작 + 10Hz 텔레메트리 TX 출력
```
> 시뮬레이터는 보정된 ADC 모델(전류 ~ PWM duty 추종, 온도 ~35°C)을 사용해
> 결함 없이(`flt:0`) 동작하며 전류제어 동작을 관찰할 수 있습니다.

### 프로토콜 규격 테스트
```bash
# 프레임 16진 출력(검증용)
python3 tools/pc_send_cmd.py frame set_motion --max_rpm 1500 --accel_rpm_s 4000
python3 tools/pc_send_cmd.py frame camera --delay_us 100 --width_us 50
python3 tools/pc_send_cmd.py frame set_current --ma 1200

# 실제 RS232 송신 (pyserial 필요)
python3 tools/pc_send_cmd.py send --port /dev/ttyUSB0 --baud 115200 set_current --ma 1200
```

### STM32G474 (CubeIDE) 타깃
- `src/platform/platform_stm32g474_cubeide.c` 의 HAL 콜백(UART/ADC-DMA/TIM)에 CubeMX 생성 핸들을 연결.
- `include/config.h` 의 임계값/게인/핀 정의를 보드에 맞게 조정.

---

## 5. 스택

C11 · STM32G474 (CubeIDE HAL/LL) · CMake · POSIX 시뮬레이터 · Python(pyserial)

원본 경력: ㈜파크시스템스 — WLI Controller (약 11개월, 이후 인텔(Intel) 납품) · 프로젝트 목록: https://my-page-lee.vercel.app/
