# mongo – консоль управления (на сервере, где установлен mongodb)
show dbs # показывает базы данных
use shop # подключается (и создает если надо) базу данных
# создадим коллекцию и добавляет элементы
db.products.insertOne({name:"A Book", price: 29, details: {pages: 500}})
db.products.insertOne({name:"B Book", price: 100, details: {pages: 12}})
# выведем все данные коллекции и отформатирует
db.products.find().pretty()
# выведем первый элемент
db.products.findOne()
# выведем определенные данные все с name “A Book”
db.products.find({name:"A Book"}).pretty()
# выведем определенные данные все что дороже 20
db.products.find({price:{$gt:50}}).pretty()

# изменим объект updateOne / updateMany
db.products.updateOne({_id:ObjectId("672ce5012ca7c0c131c76a8b")},{$set: {price: 1000}})
# удалим объект delete/ deleteOne
db.products.deleteOne({_id:ObjectId("672ce5012ca7c0c131c76a8b")})


# возвращает сразу все данные
db.products.find({name:"A Book"}).toArray()

# применяет к каждому элементу команду
db.products.find({name:"A Book"}).forEach((prod)=>{printjson(prod)})

# возвращает нужные данные (name)
db.products.find({},{name:1}).pretty()

# Массивы 
db.products.insertOne({
"_id": ObjectId("60d5f4e01c9d440000a1b2c3"),
"name": "Smartphone",
"price": 599.99,
"reviews": []});

# Массивы обновление
db.products.updateOne(
  { _id: ObjectId("60d5f4e01c9d440000a1b2c3") },
  {
    $push: {
      reviews: {
        $each: [
          { "user": "Alice", "rating": 4, "comment": "Great phone!" },
          { "user": "Bob", "rating": 5, "comment": "Excellent performance!" }
        ]
      }
    }
  }
);

db.products.findOne(   { _id: ObjectId("60d5f4e01c9d440000a1b2c3") })

# сложный объект
db.companies.insertOne({ name: "Pche Lines Inc", isStartup: true, employees: 33, funding: 12231241242141, details: { ceo: "Max Steal"}, tags :[ { title: "super"} , {title : "prefect"}], foundingDate: new Date('2014-01-01'), insertedAt:new Timestamp()})
db.companies.find().pretty()

# Связи
db.users.insertOne({ _id: "user-01", name: "Ivan Petrov"})
db.users.findOne()
db.orders.insertOne({ time: new Timestamp() , details : "Sone description" , user : "user-01" })
db.orders.findOne()

# Сехмы

db.createCollection('posts', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['title', 'text', 'creator', 'comments'],
      properties: {
        title: {
          bsonType: 'string',
          description: 'must be a string and is required'
        },
        text: {
          bsonType: 'string',
          description: 'must be a string and is required'
        },
        creator: {
          bsonType: 'objectId',
          description: 'must be an objectid and is required'
        },
        comments: {
          bsonType: 'array',
          description: 'must be an array and is required',
          items: {
            bsonType: 'object',
            required: ['text', 'author'],
            properties: {
              text: {
                bsonType: 'string',
                description: 'must be a string and is required'
              },
              author: {
                bsonType: 'objectId',
                description: 'must be an objectid and is required'
              }
            }
          }
        }
      }
    }
  }
});

# Хороший запрос
db.posts.insertOne({title: "Some title", text: "Some text", creator: ObjectId("65d9ac4ff5793bcbf97d988f"), comments: [{text: "Coool!" , author: ObjectId("65d9ac4ff5793bcbf97d988f")}]})

# Плохой
db.posts.insertOne({title: "Some title", text: "Some text", comments: [{text: "Coool!" , author: ObjectId("65d9ac4ff5793bcbf97d988f")}]})

# индексы
# mongoimport /opt/05_mongo/persons.json -d arch -c contacts --jsonArray

db.contacts.find({"dob.age" : { $gt: 60}}).pretty() # все старше 60
db.contacts.explain().find({"dob.age" : { $gt: 60}}) # анализ запроса winning plan – COLLSCAN
db.contacts.explain("executionStats").find({"dob.age" : { $gt: 60}}) 
db.contacts.createIndex({"dob.age": -1}) # параметр указывает на порядок сортировки
db.contacts.explain("executionStats").find({"dob.age" : { $gt: 60}}) 
db.contacts.dropIndex({"dob.age": -1}) # удалим индекс
db.contacts.createIndex({"dob.age": 1, gender: 1}) # "dob.age_1_gender_1"
db.contacts.explain("executionStats").find({"dob.age" : 35, "gender" : "male" })
db.contacts.explain("executionStats").find({"dob.age" : { $gt: 20}}) # использует
db.contacts.explain("executionStats").find({"gender" : "male"}) # не использует
db.contacts.getIndexes() # возвращает перечень индексов

# текстовый индекс
db.contacts.createIndex({"location.street": "text"})
db.contacts.explain("executionStats").find({$text : {$search : "road"}})
db.contacts.find({$text : {$search : "road"}}).count() # по одному слову
db.contacts.find({$text : {$search : "road street"}}).count() # по вхождению одного из слов
db.contacts.find({$text : {$search : "\"road street\""}}).count() # по вхождению фразы


# группировка 

db.contacts.aggregate([{$match : {gender : "male"} }, {$group : { _id:"$nat" , totalAge : {$avg : "$dob.age"}}}, { $sort : { totalAge : -1} }])
db.orders.aggregate([{ $lookup: { from: "users" , localField: "user" , foreignField : "_id" , as: "order_users"}}]).pretty()