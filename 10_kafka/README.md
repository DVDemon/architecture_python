# Kafka — потоковая обработка

Примеры работы с Apache Kafka для курса «Архитектура программных систем».

## Описание

Демонстрация Producer и Consumer для Kafka:

- **client.py** — Producer: отправляет сообщения в топик `topic_2`
- **server.py** — Consumer: читает сообщения из топика `topic_2`

Требуется кластер Kafka (например, `kafka1:9092`, `kafka2:9092`).

## Запуск

```bash
# Терминал 1 — Consumer (сначала запустить)
python server.py

# Терминал 2 — Producer
python client.py
```

## Зависимости

```bash
pip install confluent-kafka
```

## Конфигурация

В коде указаны брокеры: `kafka1:9092,kafka2:9092`. Измените при необходимости.
