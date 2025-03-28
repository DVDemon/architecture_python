# create table
CREATE TABLE IF NOT EXISTS Author (id INT NOT NULL,first_name VARCHAR(256),last_name VARCHAR(256),email VARCHAR(256), title VARCHAR(1024));

# show tables
\dt+

# create sequence
CREATE SEQUENCE IF NOT EXISTS author_id_seq START 1;

SELECT * from Author;

select NEXTVAL('author_id_seq');

INSERT INTO Author(id,first_name,last_name,email,title) 
VALUES ( NEXTVAL('author_id_seq'),'Petr','Petrov','pp@yandex.ru','mr');

INSERT INTO Author(id,first_name,last_name,email,title) VALUES ( NEXTVAL('author_id_seq'),'Andr','Ezhoff','pp@yandex.
ru','mr');

drop table Author;
select * from Author where id<3;
select * from Author where first_name='Petr';
select * from Author where first_name='Petr' OR first_name='Andr';

# https://tatiyants.com/pev/#/plans
EXPLAIN (ANALYZE, COSTS, VERBOSE, BUFFERS, FORMAT JSON) select * from Author where first_name='Petr' OR first_name='Andr';

explain (analyze) select * from Author where first_name='Petr' OR first_name='Andr';
explain (analyze) select * from Author where id<3;

CREATE UNIQUE INDEX author_id ON Author (id);

\d Author

create index fn_ln on Author(first_name);

drop index fn_ln;

create index fn_ln on Author(first_name,last_name);

SELECT * FROM Author WHERE first_name LIKE 'P%' OR last_name LIKE 'E%';


create index users_fn_ln on Users(first_name text_pattern_ops, last_name text_pattern_ops);
explain (ANALYZE, COSTS, VERBOSE, BUFFERS, FORMAT JSON) select * from Users where first_name like 'Car%' and last_name like 'L%';