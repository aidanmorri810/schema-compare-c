CREATE TABLE film_category (
    film_id integer NOT NULL,
    category_id integer NOT NULL,
    last_update timestamp NOT NULL DEFAULT now(),
    CONSTRAINT film_category_pkey PRIMARY KEY (film_id, category_id),
    CONSTRAINT film_category_film_id_fkey
        FOREIGN KEY (film_id) REFERENCES film(film_id),
    CONSTRAINT film_category_category_id_fkey
        FOREIGN KEY (category_id) REFERENCES category(category_id)
);

CREATE TRIGGER last_updated
    BEFORE UPDATE ON film_category
    FOR EACH ROW
    EXECUTE PROCEDURE last_updated();
