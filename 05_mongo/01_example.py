from pymongo import MongoClient
import json
# Connect to MongoDB
client = MongoClient('mongodb://mongodb:27017/')
db = client['mydatabase']  # Specify the database name
collection = db['mycollection']  # Specify the collection name

document = {
    'name': 'Jirome K Jirome',
    'age': 10,
    'email': 'kj@example.com'
}
# Insert the document into the collection 

result = collection.insert_one(document)
# Check if the insertion was successful
if result.acknowledged:
    print('Document inserted successfully.')
    print('Inserted document ID:', result.inserted_id) 
else:
    print('Failed to insert document.')

# Retrieve documents based on specific conditions
query = {
    'age': {'$gte': 29},  # Retrieve documents where age is greater than or  equal to 29
}
documents = collection.find(query)
# Iterate over the retrieved documents for document in documents:
for document in documents:
    json_document = json.dumps(document, indent=2, default=str)
    print(json_document)
