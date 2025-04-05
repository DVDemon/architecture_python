from fastapi import FastAPI, Depends, HTTPException
from pymongo import MongoClient
from pydantic import BaseModel
import os
import json

# Подключение к MongoDB
client = MongoClient(host=['mongodb:27017'])

# Выбор базы данных
db = client['arch']

# Выбор коллекции
collection = db['users']

# Модель SQLAlchemy
class User(BaseModel):
    id: str
    first_name: str
    last_name: str

app = FastAPI()



# Маршрут для создания пользователя
@app.post("/users/", response_model=User)
def create_user(user: User):

    insert_result = collection.insert_one(user.__dict__)
    print(f"User inserted with id: {insert_result.inserted_id}")
    return user

# Маршрут для получения пользователя по id
@app.get("/users/{user_id}", response_model=User)
def get_user(user_id: str):
    query = {"id": user_id}
    result = collection.find_one(query)

    if result:
        print(f"User found: {result}")
        return result
    else:
        print("User not found")
        raise HTTPException(status_code=404, detail="User not found")


# Маршрут для получения всех пользователей
@app.get("/users/", response_model=list[User])
def get_all_users():
    result = collection.find() 
    # users = db.query(User).all()
    return result

# Запуск сервера
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)