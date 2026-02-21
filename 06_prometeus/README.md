# Prometheus — метрики и мониторинг

Пример интеграции приложения с Prometheus для сбора метрик для курса «Архитектура программных систем».

## Описание

FastAPI приложение с middleware для экспорта метрик в формате Prometheus:

- **Счётчик** `http_requests_total` — количество HTTP-запросов по method и endpoint
- **Гистограмма** `http_request_duration_seconds` — задержка обработки запросов

Метрики доступны на порту **9100**.

## Endpoints

- `GET /hello` — пример эндпоинта (задержка ~0.1 с)
- `GET /goodbye` — пример эндпоинта (задержка ~0.2 с)

## Запуск

```bash
uvicorn main:app --host 0.0.0.0 --port 8000
```

## Просмотр метрик

```bash
curl http://localhost:9100/metrics
```

## Интеграция с Prometheus

Добавьте в `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'fastapi'
    static_configs:
      - targets: ['localhost:9100']
```
