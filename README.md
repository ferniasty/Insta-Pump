# Rave Academy – Instant Pump

Darmowa wtyczka od Rave Academy: efekt „sidechain” bez routingu. Bas (albo pady, reverb, cokolwiek)
ścisza się na każdą stopę i wraca płynnym łukiem, w rytm tempa projektu.

## Jak zbudować paczkę do rozdania (za darmo)

**Na swoim Macu (polecane, $0, bez limitów):** kliknij dwukrotnie `Build-macOS.command`.
Skrypt zbuduje wtyczkę (Apple Silicon + Intel), zainstaluje ją u Ciebie i zrobi paczkę
`dist/InstantPump-Mac.zip` (VST3 + AU + instrukcja + licencja + kod źródłowy).

**Na GitHubie (mieści się w darmowych $12/mies.):** zakładka **Actions → Build Instant Pump →
Run workflow**. Mac i Windows budują się tylko na żądanie albo przy tagu `vX.Y.Z`; zwykły push
uruchamia jedynie tanie testy na Linuksie. Gotowe paczki są w **Artifacts** danego przebiegu.

Paczkę wrzucasz do MailerLite / Gumroad (pobranie za adres email).

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
