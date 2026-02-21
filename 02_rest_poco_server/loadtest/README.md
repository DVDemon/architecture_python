# Load test для /api/v1/helloworld

Нагрузочное тестирование с помощью [wrk](https://github.com/wg/wrk).

## Требования

- wrk (локально) или Docker

## Запуск

### Сервер уже запущен (localhost)

```bash
./run.sh
```

Параметры по умолчанию: 4 потока, 100 соединений, 30 секунд.

### Свои параметры

```bash
./run.sh [URL] [threads] [connections] [duration]
```

Примеры:

```bash
# Короткий тест
./run.sh http://localhost:8080/api/v1/helloworld 2 50 10s

# Высокая нагрузка
./run.sh http://localhost:8080/api/v1/helloworld 8 500 60s
```

### Через docker-compose (профиль loadtest)

```bash
# Запустить сервер и нагрузочный тест
docker compose --profile loadtest up poco_template_server loadtest
```

Или по шагам:

```bash
docker compose up -d poco_template_server
sleep 3
docker compose run --rm --profile loadtest loadtest
```
