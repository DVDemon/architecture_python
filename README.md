# Архитектура программных систем — примеры

Репозиторий с примерами и лабораторными работами по курсу «Архитектура программных систем» (МАИ).

## Структура репозитория

| Директория | Тема | Описание |
|------------|------|----------|
| [00_devcontainer](00_devcontainer/) | DevContainer | Среда разработки в контейнере (FastAPI) |
| [00_docker_examples](00_docker_examples/) | Docker | Примеры Dockerfile и docker-compose |
| [01_structurizr](01_structurizr/) | Документирование | Structurizr DSL, C4-модель, ADR |
| [01_DDD](01_DDD/) | DDD | Domain-Driven Design на C++ |
| [02_rest](02_rest/) | REST API | FastAPI: от простого API до JWT |
| [02_rest_poco_server](02_rest_poco_server/) | REST (C++) | REST-сервер на POCO с JWT и метриками |
| [03_sql](03_sql/) | SQL | PostgreSQL, SQLAlchemy, партиционирование |
| [04_tpc](04_tpc/) | Транзакции | Two-Phase Commit (упрощённая модель) |
| [05_mongo](05_mongo/) | NoSQL | MongoDB, replica set |
| [06_prometeus](06_prometeus/) | Мониторинг | Prometheus-метрики в FastAPI |
| [07_redis](07_redis/) | Кэширование | Redis как кэш для PostgreSQL |
| [08_circuit_breaker](08_circuit_breaker/) | Устойчивость | Паттерн Circuit Breaker |
| [09_rabbit](09_rabbit/) | Очереди | RabbitMQ Producer/Consumer |
| [10_kafka](10_kafka/) | Стриминг | Apache Kafka Producer/Consumer |
| [11_CQRS](11_CQRS/) | CQRS | Command Query Responsibility Segregation с Kafka |

## Быстрый старт

### DevContainer (рекомендуется)

В корне репозитория есть `.devcontainer` с полным окружением:

- PostgreSQL, MongoDB, Redis
- RabbitMQ, Kafka (2 брокера)
- Prometheus, Grafana
- Structurizr Lite

```bash
# Запуск всех сервисов
docker-compose -f .devcontainer/docker-compose.yml up -d
```

### Зависимости Python

```bash
pip install -r requirements.txt
```

## Технологии

- **Python**: FastAPI, SQLAlchemy, Pydantic, Redis, PyMongo, Pika, Confluent Kafka
- **C++**: POCO (REST, JWT)
- **Инфраструктура**: Docker, PostgreSQL, MongoDB, Redis, RabbitMQ, Kafka, Prometheus

## Документация по примерам

В каждой директории есть `README.md` с описанием примера, инструкциями по запуску и основными концепциями.
