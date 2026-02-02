# 외부 연동 구성 및 통신 사양 분석 보고서 (2차)

**작성일:** 2026-02-02 (Update)
**작성자:** Antigravity (Firmware Architect)
**참조:** `260202_1800_Communication_Config_Report.md` (1차)

## 1. 개요
사용자가 ioc 파일을 수정하여 재생성한 드라이버 코드를 대상으로, 외부 연동 사양 적합성을 다시 검토하였습니다.

## 2. 변경 사항 검토 (Re-Verification)

### 2.1 SPI 설정 (W5500) - `spi.c`
- **Data Size:** `SPI_DATASIZE_8BIT` <span style="color:green">**[PASS]**</span>
    - 4-bit에서 8-bit로 **정상 수정됨**을 확인했습니다.
- **NSS (Chip Select):** `SPI_NSS_HARD_OUTPUT`, `SPI_NSS_PULSE_ENABLE` <span style="color:orange">**[WARNING]**</span>
    - **문제점:** W5500 통신 프로토콜은 `Command + Address + Data` 프레임을 보낼 때 CS(Chip Select) 핀이 계속 `Low` 상태를 유지해야 합니다.
    - STM32의 Hardware NSS Pulse 모드는 바이트 전송 사이에 CS를 잠깐 `High`로 올릴 수 있습니다 (`Pulse`). 이 경우 W5500은 통신 패킷이 끊긴 것으로 인식하여 오동작할 위험이 매우 높습니다.
    - **권장:** SPI 설정을 **Software NSS** (NSS Signal: Disabled)로 변경하고, 기존처럼 GPIO(PA4)를 직접 제어하는 것이 가장 안전합니다.

### 2.2 FDCAN 설정 - `fdcan.c`
- **설정 값:** `Prescaler=16`, `TimeSeg1=14`, `TimeSeg2=3` (모든 채널 동일)
- **분석:**
    - 여전히 FDCAN1(SECC), FDCAN2(Power), FDCAN3(IMD)가 모두 **동일한 속도**로 설정되어 있습니다.
    - **Power Module (Infypower):** 통상 **125 kbps**를 사용합니다.
    - **SECC:** 통상 **500 kbps**를 사용합니다.
    - 현재 설정값이 500kbps일 경우, **파워 모듈 통신이 불가능합니다.**
    - **권장:** 파워 모듈용 FDCAN 채널의 Baudrate 설정을 확인하고 분리하세요.

## 3. 최종 요약

| 항목 | 상태 | 코멘트 |
| :--- | :--- | :--- |
| **SPI Data Size** | **OK** | 8-bit 수정 완료. |
| **SPI CS Control** | **주의** | HW NSS Pulse 모드는 W5500 Burst 전송 오류 가능성 있음. **SW NSS 권장.** |
| **CAN Baudrate** | **확인 필요** | 모든 채널 속도가 동일함. Module별(특히 파워 모듈) 속도 차이 반영 필요. |

---
**조치 제안:**
1.  **ioc 재수정:** SPI1 NSS를 'Disable' 또는 'Software'로 변경하고 PA4를 GPIO Output으로 유지하세요.
2.  **코드 확인:** FDCAN2(파워)의 속도가 125kbps가 맞는지 다시 한 번 확인하세요. (다른 Prescaler 값 적용 필요)
