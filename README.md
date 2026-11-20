# VocalMAX

Plugin VST3 de corrección de afinación vocal en tiempo real para Windows (probado para FL Studio).

## Instalación (Windows)

1. Descarga `VocalMAX-Windows-VST3.zip` desde **Releases** (o desde la pestaña **Actions → último build → Artifacts**).
2. Descomprime y copia la carpeta `VocalMAX.vst3` a:
   `C:\Program Files\Common Files\VST3\`
3. En FL Studio: **Options → Manage plugins → Find plugins**, y luego cárgalo en un slot del Mixer de la pista de voz.

## Controles

| Control | Función |
|---|---|
| **KEY / SCALE** | Tonalidad y escala (Chromatic, Major, Minor, Harmonic/Melodic Minor, Pentatónicas, Blues, Dorian, Mixolydian, Phrygian, Lydian) |
| **INPUT** | Tipo de voz: Soprano, Alto/Tenor, Low Male, Instrument (ajusta el rango de detección) |
| **RETUNE SPEED** | 0 ms = efecto robótico/“T-Pain”; 20–50 ms = corrección natural; 100+ ms = muy sutil |
| **HUMANIZE** | Hace la corrección más lenta en notas sostenidas para que suenen naturales |
| **FLEX-TUNE** | Deja pasar la desviación natural cercana a la nota (0 = corrección estricta) |
| **TRANSPOSE** | Transposición ±12 semitonos (conserva los formantes) |
| **REF A4** | Afinación de referencia (415–465 Hz, por defecto 440) |
| **MIX** | Mezcla entre señal original y corregida |
| **OUTPUT** | Ganancia de salida |
| **Teclado** | Notas objetivo encendidas en rosa; clic para quitar/restaurar una nota |

La pantalla muestra la nota objetivo, la desviación en cents y una gráfica de tono (blanco = voz original, rosa = voz corregida).

## Cómo funciona

- Detección de tono **YIN** (con filtro de mediana contra errores de octava).
- Resíntesis **TD-PSOLA** síncrona con el periodo: cambia la afinación preservando los formantes (sin efecto “ardilla”).
- Latencia ~50 ms, reportada al host: FL Studio la compensa automáticamente (PDC).

## Compilar

GitHub Actions compila automáticamente en cada push a `main` (`.github/workflows/build-windows.yml`).
Localmente (Visual Studio 2022 + CMake 3.22+):

```
cmake -S . -B build -A x64
cmake --build build --config Release
```

JUCE 8 se descarga automáticamente con CMake FetchContent.
