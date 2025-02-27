from confluent_kafka import Consumer, KafkaError
import json

# Конфигурация Kafka Consumer
conf = {
    'bootstrap.servers': 'kafka1:9092,kafka2:9092',  # Адрес Kafka брокера
    'group.id': 'my_group',  # ID группы потребителей
    'auto.offset.reset': 'earliest'  # Начинать с самого раннего сообщения
}

# Создание Consumer
consumer = Consumer(conf)

# Подписка на топик
consumer.subscribe(['topic_2'])

# Цикл для получения сообщений
try:
    while True:
        msg = consumer.poll(1.0)  # Ожидание сообщения в течение 1 секунды

        if msg is None:
            continue
        if msg.error():
            if msg.error().code() == KafkaError._PARTITION_EOF:
                continue
            else:
                print(msg.error())
                break

        # Десериализация JSON и вывод на экран
        user_data = json.loads(msg.value().decode('utf-8'))
        print(f"Received user data: {user_data}")

except KeyboardInterrupt:
    pass

finally:
    # Закрытие Consumer
    consumer.close()