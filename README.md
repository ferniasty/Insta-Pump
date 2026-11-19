# Rave Academy – Instant Pump

Darmowa wtyczka od Rave Academy: efekt „sidechain” bez routingu. Bas (albo pady, reverb, cokolwiek)
ścisza się na każdą stopę i wraca płynnym łukiem, w rytm tempa projektu.

## Pobierz

- **Mac (VST3 + AU, Apple Silicon i Intel):** https://github.com/ferniasty/Insta-Pump/releases/latest/download/InstantPump-Mac.zip
- **Windows (VST3):** https://github.com/ferniasty/Insta-Pump/releases/latest/download/InstantPump-Windows.zip

Instrukcja instalacji jest w środku każdej paczki (plik `INSTALUJ-...txt`).
Na Macu możesz też zbudować wtyczkę sam: dwuklik w `Build-macOS.command`.

## Pokrętła

- **MIX** – jak mocno ścisza (0–100%)
- **LENGTH** – jak długo trwa ściszenie i powrót (możesz też przeciągać ekran w lewo/prawo)
- **CURVE** – wygięcie łuku powrotu (w minus: powolny „pump”, w plus: szybki powrót)
- **SHAPE** – charakter: TIGHT, PUMP, SMOOTH, CUT

Każde zbocze obwiedni jest łagodnym łukiem (najwyżej tak stromym jak 12 ms), więc żadne ustawienie nie klika.

## Dla programistów

Zbudowane w JUCE 8. Budowanie lokalne:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Testy (krzywe, stereo + kliki na prawdziwym procesorze, zrzut interfejsu):

```bash
cmake -B build -DKD_BUILD_TOOLS=ON && cmake --build build
./build/kd_envelope_test
./build/kd_snapshot_artefacts/Release/kd_snapshot preview.png
```

Nowe wydanie: wypchnij tag `vX.Y.Z` – GitHub Actions zbuduje Mac + Windows i doda paczki do Releases.

## Licencja

AGPLv3 (plik `LICENSE`). Wtyczka używa JUCE na licencji AGPLv3.
