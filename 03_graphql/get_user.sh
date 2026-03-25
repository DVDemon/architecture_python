curl -X POST http://localhost:8080/query \
-H "Content-Type: text/plain" \
-d 'query {
    get_user(id:1){
        id,
        first_name,
        last_name,
        email,
        title,
        login,
        password
    }
}'
