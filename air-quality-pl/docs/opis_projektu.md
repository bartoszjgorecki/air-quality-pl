# Szczegółowy opis projektu Air Quality Monitor

## 1. Cel projektu

Projekt Air Quality Monitor jest aplikacją desktopową napisaną w języku C++ z użyciem frameworka Qt. Głównym celem programu jest pobieranie, filtrowanie i prezentowanie danych o jakości powietrza publikowanych przez GIOS. Aplikacja ma umożliwiać przejście przez pełen scenariusz pracy z danymi: od wyboru stacji pomiarowej, przez wybór sensora, aż do obejrzenia wyników na wykresie i zapisania ich do lokalnej bazy JSON.

Drugi ważny cel projektu to odporność na błędy zewnętrznych usług. Dane pochodzą z publicznego API REST, więc aplikacja musi zakładać możliwość braku sieci, timeoutów, zmian formatu JSON albo częściowej niedostępności danych. Z tego powodu w kodzie zostały wydzielone osobne moduły odpowiedzialne za sieć, parsowanie, przechowywanie lokalne i logikę interfejsu. Taki podział zmniejsza ryzyko awarii całej aplikacji w sytuacji, gdy zawiedzie tylko jeden element.

## 2. Ogólny przebieg działania

Praca użytkownika z aplikacją przebiega w kilku krokach. Najpierw użytkownik pobiera listę wszystkich stacji pomiarowych w Polsce. Lista może być następnie zawężana na dwa sposoby: zwykłym filtrem tekstowym albo wyszukiwaniem stacji w promieniu od podanego adresu. Po wybraniu konkretnej stacji aplikacja pobiera dostępne dla niej sensory. Następnie użytkownik wybiera interesujący go sensor, na przykład PM10 albo NO2, a aplikacja pobiera serię pomiarową z API i prezentuje ją na wykresie.

Równolegle aplikacja oblicza podstawowe statystyki serii: minimum, maksimum, średnią, liczbę braków danych oraz prosty trend wzrostowy lub spadkowy. Użytkownik może też zapisać aktualnie pobraną serię do lokalnego pliku JSON. Dzięki temu w przypadku awarii API można później wczytać historię z bazy lokalnej i nadal pokazać dane na wykresie.

## 3. Architektura i podział na moduły

Projekt został podzielony na kilka logicznych części.

### 3.1. Warstwa modelu

W katalogu `src/model` znajdują się podstawowe struktury danych. `MeasurementPoint` reprezentuje pojedynczy punkt pomiarowy z datą i wartością, a `Stats` przechowuje wynik analizy całej serii. To jest najprostsza warstwa projektu, ale bardzo ważna, bo z tych struktur korzysta zarówno analizator, jak i lokalna baza danych.

### 3.2. Warstwa analizy

Moduł `Analyzer` odpowiada za liczenie statystyk na podstawie wektora punktów pomiarowych. To tutaj wyznaczane są wartości minimalne i maksymalne, średnia, liczba brakujących wartości oraz prosty trend. Dzięki wydzieleniu tej logiki do osobnej klasy dało się łatwo napisać testy jednostkowe i nie mieszać obliczeń z kodem interfejsu.

### 3.3. Warstwa przechowywania lokalnego

Klasa `LocalDb` obsługuje lokalny plik `db.json`. Jej zadaniem jest utworzenie pustej bazy przy pierwszym uruchomieniu, dopisywanie lub aktualizowanie serii dla wybranego sensora oraz odczyt historii z zadanego zakresu czasu. Zapis jest realizowany przez `QSaveFile`, co zmniejsza ryzyko uszkodzenia pliku przy niepełnym zapisie. Ta warstwa nie zna interfejsu użytkownika, tylko wykonuje operacje na danych.

### 3.4. Warstwa sieci i parserów

`GiosClient` odpowiada za połączenie z API GIOS. Wysyła żądania HTTP, pilnuje timeoutów i odbiera surowe odpowiedzi. Surowy JSON nie jest jednak od razu przekazywany dalej. Najpierw odpowiedź trafia do `GiosParsers`, które zamieniają różne warianty odpowiedzi na ujednolicony format wykorzystywany przez resztę programu.

To rozdzielenie jest istotne z dwóch powodów. Po pierwsze, parser można testować niezależnie od sieci. Po drugie, gdyby API zaczęło zwracać trochę inny JSON, poprawki zwykle ograniczą się do jednego modułu, bez naruszania reszty aplikacji.

### 3.5. Warstwa aplikacyjna

`AppController` pełni rolę centralnego koordynatora. Łączy interfejs QML z klientem REST, analizatorem i bazą lokalną. To on decyduje, kiedy pobrać stacje, kiedy wyczyścić listę sensorów, kiedy zapisać serię do bazy, a kiedy przełączyć aplikację do trybu offline. Dzięki temu QML nie zawiera logiki biznesowej i zajmuje się głównie prezentacją.

### 3.6. Warstwa interfejsu

Główny interfejs został zapisany w `qml/main.qml`. Zawiera lewą kolumnę z listą stacji i sensorów oraz prawą część z kartami danych, wykresem i mapą. Użytkownik może filtrować stacje, uruchamiać wyszukiwanie po promieniu, przełączać się między widokiem wykresu i mapy oraz korzystać z historii lokalnej.

Wykres jest interaktywny: można go przybliżać kółkiem myszy, przesuwać przeciąganiem i resetować dwuklikiem. Dodatkowo kliknięcie w pojedynczy punkt pokazuje dokładną wartość i czas pomiaru. Mapa również jest interaktywna i pozwala na przesuwanie oraz przybliżanie widoku.

## 4. Obsługa wyjątków i odporność na błędy

Jednym z ważniejszych założeń projektu była odporność na sytuacje awaryjne. W praktyce zrealizowano to na kilku poziomach.

Na poziomie sieci `GiosClient` raportuje błędy żądań, niepoprawne kody odpowiedzi i timeouty. Na poziomie parserów niepoprawny format JSON kończy się wyjątkiem `std::runtime_error`, co pozwala odróżnić błąd sieci od błędu treści odpowiedzi. W `AppController` wszystkie operacje pobierania i zapisu są otoczone blokami `try/catch`, więc awaria pojedynczej akcji nie kończy pracy całego programu.

Lokalna baza także zwraca czytelne komunikaty błędów, na przykład gdy plik jest uszkodzony albo nie udało się go otworzyć. Jeżeli pobieranie online się nie uda, aplikacja przechodzi w tryb offline i informuje użytkownika, czy dla wybranego sensora istnieje już lokalna historia. Dzięki temu użytkownik nie zostaje bez informacji, tylko może przełączyć się na dane zapisane wcześniej.

## 5. Wielowątkowość i responsywność

Projekt wykorzystuje podejście asynchroniczne i wielowątkowe, aby interfejs nie zawieszał się przy większych odpowiedziach z API. Same żądania HTTP są wykonywane nieblokująco przez `QNetworkAccessManager`. To oznacza, że wątek interfejsu nie czeka bezczynnie na odpowiedź serwera.

Dodatkowo surowa odpowiedź JSON nie jest parsowana w głównym wątku, tylko przez `QtConcurrent` w wątku roboczym. To ważne zwłaszcza przy dłuższych listach stacji i większych odpowiedziach pomiarowych. Dzięki temu okno, wykres i mapa pozostają aktywne nawet wtedy, gdy aplikacja przetwarza dane w tle.

Na mapie zastosowano jeszcze jeden mechanizm: odświeżanie kolorów dla stacji wykorzystuje wiele równoległych zapytań i token żądania. Jeśli użytkownik szybko zmieni filtr albo wybierze inny sensor, stare odpowiedzi są ignorowane i nie nadpisują już nowego stanu aplikacji.

## 6. Testy jednostkowe

W projekcie znajdują się testy jednostkowe uruchamiane przez Qt Test. Testowane są trzy główne obszary: analizator statystyk, lokalna baza danych oraz parsery odpowiedzi GIOS. Testy parserów są szczególnie istotne, bo API bywa niejednorodne i w praktyce może zwracać różne nazwy pól. Dzięki temu łatwo sprawdzić, czy aplikacja nadal poprawnie rozpoznaje stacje, sensory i dane pomiarowe po zmianie formatu odpowiedzi.

Testy lokalnej bazy weryfikują zapis, aktualizację i odczyt historii, a także obsługę uszkodzonego pliku JSON. Testy analizatora sprawdzają poprawność średniej, minimum, maksimum i trendu. Taki zestaw nie obejmuje interfejsu graficznego, ale daje dobrą ochronę dla najważniejszej logiki projektu.

## 7. Wzorce projektowe

W projekcie można wskazać kilka wzorców projektowych. `AppController` realizuje rolę `Facade` albo `Application Controller`, ponieważ skupia w jednym miejscu koordynację operacji widocznych z poziomu GUI. `GiosClient` i `LocalDb` pełnią funkcję `Repository` lub źródeł danych. Sygnały i sloty Qt tworzą naturalny odpowiednik wzorca `Observer`, bo interfejs reaguje na zmiany stanu obiektów C++. `GiosParsers` można traktować jak `Adapter`, ponieważ dopasowują zewnętrzny format JSON do wewnętrznego modelu programu.

## 8. Podprojekty pośrednie

W katalogu `podprojekty` przygotowano też dwie prostsze wersje aplikacji. Pierwsza pokazuje absolutne minimum potrzebne do zbudowania okna Qt Quick z jednym przyciskiem. Druga jest etapem przejściowym: potrafi pobrać stacje, sensory i listę pomiarów, ale nie ma jeszcze wykresu, mapy ani lokalnej bazy offline. Dzięki temu można prześledzić, jak projekt rósł krok po kroku od prostego interfejsu do pełnej aplikacji.

## 9. Podsumowanie

Najważniejszą zaletą projektu jest połączenie kilku zagadnień typowych dla aplikacji inżynierskich: komunikacji z REST API, pracy asynchronicznej, prostego przetwarzania danych, lokalnego zapisu oraz interfejsu graficznego. Jednocześnie kod został podzielony na moduły, które da się rozwijać i testować osobno. Dzięki temu aplikacja nie jest tylko jednorazowym demonstratorem, ale ma strukturę pozwalającą na dalszą rozbudowę.
