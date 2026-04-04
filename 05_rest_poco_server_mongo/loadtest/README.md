# Load test для CRUD `User` / `Organization` (MongoDB)

Скрипт делает подготовку данных и затем гоняет нагрузки через [wrk](https://github.com/wg/wrk):

1. Создаёт/доделывает до `10000` записей в `/api/v1/users` (если их меньше)
2. Сначала выполняет `PUT` случайных `users`
3. Затем делает `GET` поисковые запросы с query-параметрами (`first_name`, `last_name`, `organization`)
4. Добавочно выполняет `GET /api/v1/users/{id}` (по случайным id) для измерения lookup скорости

## Требования

- `python3`
- `wrk` (локально) или Docker (image `satishsverma/wrk`)

Подготовка вызывает `GET /api/v1/users` и ожидает **полный** список пользователей (для подсчёта и проверки). В сервере список должен отдаваться с обходом курсора MongoDB (`OpMsgCursor` / все батчи), иначе MongoDB отдаёт только первую порцию (~101 документ) и скрипт ошибочно решит, что не хватает данных до `target_users`.

## Запуск

Сервер должен быть запущен.

```bash
./run.sh http://localhost:8080 4 100 20s 20s 1000
```

Параметры по умолчанию:

- `base_url=http://localhost:8080`
- `threads=4`
- `connections=100`
- `update_duration=20s`
- `search_duration=20s`
- `update_count=1000`

