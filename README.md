# Сайт «Онлайн-новини»

Курсова робота з ОП, варіант 10. Реалізація — C++17, CMake, HTTP-бібліотека
[cpp-httplib](https://github.com/yhirose/cpp-httplib).

## Структура

```
news-app/
├── CMakeLists.txt          # корінь, тягне cpp-httplib через FetchContent
├── domain/                 # статична бібліотека news_domain (модель)
│   ├── CMakeLists.txt
│   ├── include/news/*.h
│   └── src/*.cpp
├── server/                 # бінарник news_server (HTTP)
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp
│       ├── Router.cpp/.h   # маршрути
│       ├── Html.cpp/.h     # шаблон сторінки + escape
│       ├── Sessions.cpp/.h # сесії в пам'яті
│       └── Auth.cpp/.h     # хеш пароля з сіллю
├── docs/                   # UML-діаграма, пояснювальна записка
├── data/news.db            # створюється при першому запуску
└── README.md
```

## Збірка та запуск

```bash
cmake -S . -B build
cmake --build build -j
./build/bin/news_server
```

Або одним рядком:

```bash
cmake --build build --target run
```

Параметри:

```
news_server [--host HOST] [--port PORT] [--data FILE]
```

За замовчуванням `127.0.0.1:8080`, файл бази — `data/news.db`.

## Перший запуск

При порожній базі додаються тестові категорії, теги і кілька новин.
Готовий обліковий запис: логін `demo`, пароль `demo123`.

Далі: http://127.0.0.1:8080.

## Функції

- реєстрація/вхід автора;
- додавання, редагування, видалення своєї новини;
- стрічка з фільтрами: пошук за фрагментом назви, рубрика, тег, автор, період;
- сортування за датою, заголовком або іменем автора у двох напрямках;
- сторінки рубрики `/category/{slug}`, тегу `/tag/{slug}`, автора `/author/{username}`;
- збереження у текстовий файл `data/news.db` після кожної зміни.

## Залежності

| Що                              | Версія    |
|---------------------------------|-----------|
| CMake                           | ≥ 3.16    |
| Компілятор C++17                | будь-який |
| cpp-httplib (FetchContent)      | v0.18.1   |
| Git (для FetchContent)          | будь-який |
