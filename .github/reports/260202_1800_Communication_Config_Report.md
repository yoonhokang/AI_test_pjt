# 외부 연동 구성 및 통신 사양 분석 보고서

**작성일:** 2026-02-02
**작성자:** Antigravity (Firmware Architect)

## 1. 외부 연동 구성품 통신 사양 (Expected)

기존 드라이버 코드를 기반으로 분석된 각 모듈별 통신 사양은 다음과 같습니다.

| 구성품 | 통신 방식 | 프로토콜 | 상세 설정 (예상) | 참고 파일 |
| :--- | :--- | :--- | :--- | :--- |
| **SECC** ( 충전 제어) | CAN (FDCAN1) | Proprietary (BSP) | Baudrate: **500 kbps** (Typical)<br>ID: `0x600`(Tx), `0x610`(Rx) | `secc_driver.h` |
| **Power Module** (파워) | CAN (FDCAN2) | Infypower (CAN 2.0B) | Baudrate: **125 kbps** (Infy Spec)<br>ExtID: `0x180050xx` | `infy_power.h` |
| **IMD** (절연 감시) | CAN (FDCAN3) | - | Baudrate: **250 kbps** or **500 kbps** (Check Spec)<br>StdID | `app_main.c` (implied) |
| **Meter** (전력량계) | UART (USART) | Modbus RTU | Baudrate: **115200** or 9600 (Dev dependent)<br>8 Data, No Parity, 1 Stop | `meter_driver.h` |
| **Ethernet** (W5500) | SPI (SPI1) | W5500 Driver | Mode: 0 or 3<br>Data Size: **8-bit**<br>CS Pin: PA4 | `w5500_driver.h` |
| **Debug CLI** | UART (USART1) | CLI Protocol | Baudrate: **115200**<br>8N1 | `cli.c` |

---

## 2. 현재 ioc 생성 코드 검토 (Current Configuration)

`Core/Src` 폴더 내 생성된 초기화 코드를 검토한 결과입니다.

### 2.1 SPI 설정 (Ethernet/W5500) - `spi.c`
- **Instance:** `SPI1`
- **Mode:** Master
- **Data Size:** `SPI_DATASIZE_4BIT` <span style="color:red">**[WARNING]**</span>
    - W5500은 8-bit 통신을 사용합니다. 4-bit 설정은 오동작을 유발합니다.
- **BaudRatePrescaler:** `SPI_BAUDRATEPRESCALER_8` (PCLK에 따라 속도 결정, 적절함)
- **Pin:** PA6(MISO), PA7(MOSI), PB3(SCK)
- **CS Pin:** `w5500_driver.h`는 **PA4**를 예상하지만, `spi.c`에서 CS 핀 제어는 `Soft` 방식이므로 GPIO 설정`MX_GPIO_Init`을 확인해야 합니다.

### 2.2 UART 설정 - `usart.c`
- **USART1 (CLI?):** 115200 bps, 8N1 (PC4/PC5) - **적절**
- **USART2 (Unused?):** 115200 bps, 8N1 (PA2/PA3)
- **USART3 (Meter?):** 115200 bps, 8N1 (PB10/PB11) - **적절** (단, 실제 미터기 스펙이 9600이면 수정 필요)

### 2.3 FDCAN 설정 - `fdcan.c`
모든 채널(FDCAN1, 2, 3)이 동일한 타이밍으로 설정되어 있습니다.
- **Clock:** PCLK1 (System Clock 의존)
- **Nominal Prescaler:** 16
- **TimeSeg1:** 14, **TimeSeg2:** 3
- **계산:** `14 + 3 + 1 = 18 Tq`?
    - 만약 PCLK1 = 170MHz 라면, `170MHz / 16 / 18` = 약 590kHz? (비표준)
    - **각 모듈(SECC, Power, IMD)마다 요구하는 Baudrate가 다를 수 있는데(500k vs 125k), 현재 일괄 설정되어 있어 수정이 필요합니다.**

---

## 3. 검토 의견 및 조치 사항

### [Critical Issues]
1.  **SPI1 Data Size:** `4BIT` -> `8BIT`로 변경해야 합니다. (`spi.c`)
2.  **FDCAN Baudrate:**
    - **Power Module(Infy)**은 보통 **125kbps**를 사용합니다. 현재 설정이 500kbps라면 통신이 불가능합니다.
    - **SECC**와 **IMD**의 속도 스펙을 확인 후, `MX_FDCANx_Init` 함수별로 `Prescaler` 값을 다르게 설정해야 합니다.

### [Minor Checks]
1.  **Meter UART:** 사용하는 실제 전력량계의 Baudrate가 115200인지 9600인지 확인 필요.
2.  **W5500 CS Pin:** `main.c`의 `MX_GPIO_Init`에 **PA4** 핀이 출력으로 설정되어 있는지 확인 필요.

**결론:** ioc 파일에서 **SPI 데이터 사이즈(8bit)**와 **각 CAN 채널별 Baudrate**를 개별적으로 수정하여 코드를 다시 생성할 것을 권장합니다.
