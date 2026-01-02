CREATE TABLE film_actor (
    actor_id integer NOT NULL,
    film_id integer NOT NULL,
    last_update timestamp NOT NULL DEFAULT now(),
    CONSTRAINT film_actor_pkey PRIMARY KEY (actor_id, film_id),
    CONSTRAINT film_actor_actor_id_fkey
        FOREIGN KEY (actor_id) REFERENCES actor(actor_id),
    CONSTRAINT film_actor_film_id_fkey
        FOREIGN KEY (film_id) REFERENCES film(film_id)
);

CREATE INDEX idx_fk_film_id ON film_actor (film_id);

CREATE TRIGGER last_updated
    BEFORE UPDATE ON film_actor
    FOR EACH ROW
    EXECUTE PROCEDURE last_updated();
