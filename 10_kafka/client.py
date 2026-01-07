from confluent_kafka import Producer
import json

# Данные о пользователе


# Конфигурация Kafka Producer
conf = {
    'bootstrap.servers': 'kafka1:9092,kafka2:9092'  # Адрес Kafka брокера
}

# Создание Producer
producer = Producer(**conf)

# Функция для обработки результатов доставки сообщения
def delivery_report(err, msg):
    if err is not None:
        print(f'Message delivery failed: {err}')
    else:
        print(f'Message delivered to {msg.topic()} [{msg.partition()}]')

# Преобразование данных в JSON и отправка в Kafka
try:
    for i in range(100000):
        user_data = {
            "id": i,
            "name": "John Doe",
            "email": "john.doe@example.com"
        }
        producer.produce(
            topic='topic_2',  # Название топика
            value=json.dumps(user_data),
            callback=delivery_report
        )
    producer.flush()  # Ожидание доставки всех сообщений
except Exception as e:
    print(f'Error: {e}')