Projekt Stacje Pomiarowe
======================

Opis projektu
-------------
Aplikacja "Stacje Pomiarowe" to narzędzie desktopowe stworzone w ramach projektu na zajęcia z Języków Programowania Obiektowego. Służy do wyszukiwania stacji pomiarowych jakości powietrza w Polsce, pobierania danych z API GIOŚ (Główny Inspektorat Ochrony Środowiska) oraz wizualizacji danych pomiarowych. Użytkownik może wyszukiwać stacje po lokalizacji, przeglądać historię sesji oraz analizować dane w formie wykresów.

Funkcjonalności
---------------
- **Wyszukiwanie stacji pomiarowych**: Wprowadź nazwę miasta lub adres (np. "Warszawa" lub "ul. Marszałkowska 10, Warszawa") i opcjonalny promień wyszukiwania (w kilometrach).
- **Geokodowanie**: Automatyczne pobieranie współrzędnych geograficznych dla podanej lokalizacji za pomocą Nominatim (OpenStreetMap).
- **Pobieranie danych**: Dane o stacjach, sensorach, pomiarach i indeksie jakości powietrza pobierane z API GIOŚ.
- **Historia sesji**: Zapisywanie sesji wyszukiwania (lokalizacja, stacje, pomiary) w lokalnych plikach JSON.
- **Wizualizacja danych**: Wykresy liniowe dla wybranych sensorów i dat, z obliczonymi statystykami (min, max, średnia, trend).
- **Tryb offline**: Możliwość przeglądania zapisanych danych historycznych bez połączenia z internetem.
- **Interfejs użytkownika**: Intuicyjny interfejs oparty na Qt, z listą stacji, wyborem sensorów, kalendarzem i wykresami.

Struktura projektu
------------------
- **main.cpp**: Punkt wejścia aplikacji, inicjalizacja QApplication i MainWindow.
- **mainwindow.h/cpp**: Główny interfejs aplikacji, obsługa wyszukiwania, geokodowania i listy stacji.
- **window_2_data_vis.h/cpp**: Okno wizualizacji danych, zarządzanie sensorami, pomiarami i wykresami.
- **historymanager.h/cpp**: Zarządzanie historią sesji, zapisywanie i wczytywanie danych w formacie JSON.
- **mainwindow.ui**: Plik UI dla głównego okna (wyszukiwanie, lista stacji).
- **window_2_data_vis.ui**: Plik UI dla okna wizualizacji (wybór sensorów, kalendarz, wykresy).
- **JPO_projekt_2.pro**: Plik projektu Qt, określa zależności i konfigurację.

Wymagania
---------
Szczegółowe wymagania systemowe i zależności znajdują się w pliku `requirements.txt`.

Instrukcja instalacji
---------------------
1. Zainstaluj Qt 5 lub nowszy (zalecana wersja: Qt 5.15).
2. Zainstaluj wymagane biblioteki (patrz `requirements.txt`).
3. Sklonuj repozytorium projektu.
4. Otwórz plik `JPO_projekt_2.pro` w Qt Creator.
5. Skompiluj i uruchom projekt.

Użycie
------
1. Uruchom aplikację.
2. W polu wyszukiwania wpisz lokalizację (np. "Kraków" lub "ul. Floriańska 1, Kraków").
3. Opcjonalnie podaj promień wyszukiwania w kilometrach (promień zostanie wykorzystany w momencie gdy miasta nie będzie w bazie API GIOŚ).
4. Kliknij "Szukaj", aby pobrać listę stacji pomiarowych.
5. Wybierz stację z listy, aby otworzyć okno wizualizacji.
6. W oknie wizualizacji wybierz sensory, daty i typ wykresu, a następnie kliknij "Wyświetl dane".
7. Aby przeglądać historię, kliknij przycisk "HISTORIA" w głównym oknie i wybierz sesję.

Znane ograniczenia
------------------
- Aplikacja wymaga połączenia z internetem do pobierania danych z API GIOŚ i Nominatim (tryb offline obsługuje tylko dane historyczne).
- Wykresy obsługują tylko typ liniowy.
- API GIOŚ może mieć ograniczenia dotyczące liczby zapytań.

Autorzy
-------
- Karol Kaszuba (projekt studencki na zajęcia JPO)

Kontakt
-------
W razie pytań lub problemów proszę kontaktować się poprzez karol.kaszuba@student.put.poznan.pl