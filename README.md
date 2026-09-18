# Orua D3

Command-line tools for decoding and encoding immersive 3D audio in the
**Auro-Codec v3** and **Auro-CX** formats on Windows.

> **This project is not affiliated with GOER DYNAMICS BV.**
> *AURO-3D®* and the Auro-3D symbol are registered trademarks of
> **GOER DYNAMICS BV** (the format was originally developed by
> **Auro Technologies**). This is an independent interoperability and
> format-research project. It is not connected to, endorsed by, sponsored
> by, or approved by GOER DYNAMICS BV, Auro Technologies, Galaxy Studios,
> or any of their affiliates.

---

## What is here

| Path | Description |
| --- | --- |
| `tools/auro3d-decode` | Decoder for Auro-Codec v3 and Auro-CX streams |
| `tools/auro3d-encode` | Encoder for Auro-Codec v3 |
| `tools/common` | Shared helpers (build date, console helpers) |

## Decoder highlights

- Inputs: `WAV`, `FLAC`, `MKV`, `MP4`, `M2TS`, `DTS-HD MA`, and raw
  interleaved PCM24 (`--raw`).
- Auro-Codec v3 decoding from 2.0 up to 13.1 / 7.1+4H / 7.1+5H+1T.
- Auro-CX from MP4 (lossless and near-lossless AWC/ICC paths).
- Output as WAV, FLAC (up to 8 channels), Wave64 (`.w64`), or RF64/BW64
  for files larger than 4 GiB.
- Auro-Matic v3 upmixing and downmixing (`--dsp-strength`,
  `--dsp-output-channels` / `--dsp-output-layout`).
- Binaural rendering to HRTF stereo, including the stateful AHP/AM4HP
  HPV2 graph (`--binaural`, `--binaural-hpv2`, `--binaural-am4hp-preset`).
- Diagnostics: `--probe`, `--channel-diagram`, `--mono-tracks`.
- Deterministic regression suite under
  `tools/auro3d-decode/regression`.

## Encoder highlights

- Input: multichannel PCM24 WAV or FLAC.
- Explicit channel order via `--input-channel-order`, e.g.
  `FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS`.
- Layouts: 2.0, 2.1, 5.1, 7.1, 5.1_4H, 7.1_4H, 5.1_4H_1T, 7.1_4H_1T,
  7.1_5H_1T.
- Native profiles 1..5, deterministic serial mode (`--threads 1`),
  dither/seed control, per-channel dynamic gains.
- Output WAV or FLAC.

---

## Requirements

- Windows 10/11 x64.
- Visual Studio 2022 (Community/Professional/Enterprise or Build Tools)
  with the *Desktop development with C++* workload.
- `ffmpeg` and `ffprobe` available in `PATH` (input demuxing and FLAC
  handling).

## Build

```bat
cd tools\auro3d-decode
cmd /c build.bat

cd ..\auro3d-encode
cmd /c build.bat
```

Output binaries:

```text
bin\Release\orua3d-decode.exe
bin\Release\orua3d-encode.exe
```

The scripts locate `VsDevCmd.bat` automatically; adjust the paths at the
top of each `build.bat` if your Visual Studio is installed elsewhere.

## Usage

Decode Auro-Codec v3 to WAV:

```bat
bin\Release\orua3d-decode.exe -i input.flac -o output.wav
```

Probe an input without writing audio:

```bat
bin\Release\orua3d-decode.exe --probe -i input.mkv
```

Decode and render to binaural stereo:

```bat
bin\Release\orua3d-decode.exe -i input.mkv -o binaural.wav --binaural-hpv2
```

Decode Auro-CX to RF64/Wave64:

```bat
bin\Release\orua3d-decode.exe -i input.mp4 -o output.w64 --output-format w64
```

Full option list:

```bat
bin\Release\orua3d-decode.exe --help
```

Encode PCM to Auro-Codec v3:

```bat
bin\Release\orua3d-encode.exe -i input_7_1_4H.wav ^
    --input-channel-order FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS ^
    -o output_auro.wav
```

Full option list:

```bat
bin\Release\orua3d-encode.exe --help
```

## Regression

```bat
python tools\auro3d-decode\regression\run_regression.py check
python tools\auro3d-decode\regression\run_regression.py check --cx-only
```

The suite decodes the corpus, verifies exact bitstream consumption and
PCM invariants, and compares output hashes against
`regression/baseline.json`.

---

## License

This project is released under the **PolyForm Noncommercial License
1.0.0** (see [`LICENSE`](LICENSE)):

- **Noncommercial use is permitted** (personal, research, education,
  hobby, charitable, and government use).
- **Attribution is required**: you must keep the license terms and the
  `Required Notice` line.
- **Commercial use is not permitted** without a separate license from the
  copyright holder.

Copyright (c) 2026 **almirus** ([@almirus](https://github.com/almirus)).

## Legal notice

- This is an independent project, **not affiliated with GOER DYNAMICS BV,
  Auro Technologies, or their affiliates**. *AURO-3D®* is a registered
  trademark of GOER DYNAMICS BV and is used here only descriptively.
- The tools are intended for interoperability, format research,
  education, and playback of content you are legally entitled to use.
- You are responsible for complying with the laws of your jurisdiction
  and with any agreements that apply to the media you process. Do not use
  these tools to circumvent copyright protection or licensing terms.

# Orua D3

Консольные утилиты для декодирования и кодирования объёмного 3D-звука в
форматах **Auro-Codec v3** и **Auro-CX** под Windows.

> **Проект не связан с компанией GOER DYNAMICS BV.**
> *AURO-3D®* и символ Auro-3D являются зарегистрированными товарными
> знаками **GOER DYNAMICS BV** (изначально формат разработан компанией
> **Auro Technologies**). Это независимый проект по совместимости и
> исследованию формата. Он не связан с GOER DYNAMICS BV, Auro
> Technologies, Galaxy Studios и их аффилированными лицами, не
> одобрен и не спонсируется ими.

---

## Состав репозитория

| Каталог | Назначение |
| --- | --- |
| `tools/auro3d-decode` | Декодер Auro-Codec v3 и Auro-CX |
| `tools/auro3d-encode` | Кодировщик Auro-Codec v3 |
| `tools/common` | Общие вспомогательные файлы |

## Возможности декодера

- Вход: `WAV`, `FLAC`, `MKV`, `MP4`, `M2TS`, `DTS-HD MA` и «сырой»
  интерлив PCM24 (`--raw`).
- Декодирование Auro-Codec v3 от 2.0 до 13.1 / 7.1+4H / 7.1+5H+1T.
- Auro-CX из MP4 (lossless и near-lossless AWC/ICC).
- Вывод: WAV, FLAC (до 8 каналов), Wave64 (`.w64`), а также RF64/BW64
  для файлов больше 4 ГиБ.
- Апмикс/даунмикс Auro-Matic v3 (`--dsp-strength`,
  `--dsp-output-channels` / `--dsp-output-layout`).
- Бинауральный рендер в стерео, включая stateful-граф AHP/AM4HP HPV2
  (`--binaural`, `--binaural-hpv2`, `--binaural-am4hp-preset`).
- Диагностика: `--probe`, `--channel-diagram`, `--mono-tracks`.
- Детерминированный регресс в `tools/auro3d-decode/regression`.

## Возможности кодировщика

- Вход: многоканальный PCM24 WAV или FLAC.
- Явный порядок каналов через `--input-channel-order`, например
  `FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS`.
- Раскладки: 2.0, 2.1, 5.1, 7.1, 5.1_4H, 7.1_4H, 5.1_4H_1T, 7.1_4H_1T,
  7.1_5H_1T.
- Профили 1..5, детерминированный последовательный режим
  (`--threads 1`), управление дизерингом/seed, динамические усиления по
  каналам.
- Вывод WAV или FLAC.

---

## Требования

- Windows 10/11 x64.
- Visual Studio 2022 (Community/Professional/Enterprise или Build Tools)
  с рабочей нагрузкой *Разработка классических приложений на C++*.
- `ffmpeg` и `ffprobe` в `PATH` (демуксинг и работа с FLAC).

## Сборка

```bat
cd tools\auro3d-decode
cmd /c build.bat

cd ..\auro3d-encode
cmd /c build.bat
```

Готовые бинарники:

```text
bin\Release\orua3d-decode.exe
bin\Release\orua3d-encode.exe
```

Скрипты сами ищут `VsDevCmd.bat`; при необходимости поправьте пути в
начале соответствующего `build.bat`.

## Использование

Декодирование Auro-Codec v3 в WAV:

```bat
bin\Release\orua3d-decode.exe -i input.flac -o output.wav
```

Диагностика без записи аудио:

```bat
bin\Release\orua3d-decode.exe --probe -i input.mkv
```

Декодирование с бинауральным рендером:

```bat
bin\Release\orua3d-decode.exe -i input.mkv -o binaural.wav --binaural-hpv2
```

Декодирование Auro-CX в Wave64:

```bat
bin\Release\orua3d-decode.exe -i input.mp4 -o output.w64 --output-format w64
```

Полный список опций:

```bat
bin\Release\orua3d-decode.exe --help
```

Кодирование PCM в Auro-Codec v3:

```bat
bin\Release\orua3d-encode.exe -i input_7_1_4H.wav ^
    --input-channel-order FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS ^
    -o output_auro.wav
```

Полный список опций:

```bat
bin\Release\orua3d-encode.exe --help
```

## Регресс

```bat
python tools\auro3d-decode\regression\run_regression.py check
python tools\auro3d-decode\regression\run_regression.py check --cx-only
```

Набор декодирует корпус, проверяет точное потребление битстрима и
инварианты PCM, а также сравнивает хеши выходных файлов с
`regression/baseline.json`.

---

## Лицензия

Проект распространяется по лицензии **PolyForm Noncommercial License
1.0.0** (см. [`LICENSE`](LICENSE)):

- **Некоммерческое использование разрешено** (личное, исследовательское,
  образовательное, хобби, благотворительное, государственное).
- **Указание автора обязательно**: нужно сохранять текст лицензии и
  строку `Required Notice`.
- **Коммерческое использование запрещено** без отдельной лицензии от
  правообладателя.

Copyright (c) 2026 **almirus** ([@almirus](https://github.com/almirus)).

## Правовая информация

- Это независимый проект, **не связанный с GOER DYNAMICS BV, Auro
  Technologies и их аффилированными лицами**. *AURO-3D®* —
  зарегистрированный товарный знак GOER DYNAMICS BV; название
  используется здесь только в описательных целях.
- Утилиты предназначены для совместимости, исследования формата,
  обучения и воспроизведения контента, на который у вас есть законные
  права.
- Ответственность за соблюдение законодательства вашей юрисдикции и
  применимых соглашений лежит на вас. Не используйте эти утилиты для
  обхода защиты авторских прав или условий лицензирования.


