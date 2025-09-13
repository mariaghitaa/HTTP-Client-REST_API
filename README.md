# GHITA MARIA - 321 CC

## Overview

This project implements the assignment starting from the lab files for Laboratory 9: `buffer.c`, `buffer.h`, `helpers.c`, `helpers.h`, `requests.h`, and `requests.c`. I implemented the functions I had worked on in the lab and also asked a lab assistant to review my implementations to ensure correctness.

I added `compute_delete_request` which creates an HTTP `DELETE` request. I also implemented `compute_put_request` (similar to `compute_post_request`) and used it in `update_movie` to update an existing movie.

To parse server responses I used the Parson JSON library (`parson.c` and `parson.h`) from the GitHub link provided in the assignment. I chose Parson because it is simple to use; I followed tutorials to learn how to parse basic JSON structures.

To store authentication information I used global variables for the JWT token and the session cookie. The cookie is updated after each server response using the `update_cookie` function. I also used two boolean flags: `is_admin` and `is_logged_in` to track whether the user is logged in and whether they are an admin. These flags are set based on server responses.

The program structure follows the pattern of having one function per possible command. In `main` I read the command and call the corresponding function. If a command is not valid, the program prints "Unknown command".

---

## Files used / modified

* `buffer.c`, `buffer.h`
* `helpers.c`, `helpers.h`
* `requests.c`, `requests.h`
* `parson.c`, `parson.h` (third-party JSON library)

All networking helper functions from the lab were adapted to include the JWT token in request headers where required.

---

## Global state

* `jwt_token` — global string storing the JWT token for library access.
* `session_cookie` — global string storing the session cookie; updated by `update_cookie`.
* `is_admin` — flag set to `true` when the logged-in user is an admin.
* `is_user` / `is_logged_in` — flag(s) indicating a normal user is logged in.

---

## High-level design

* One function per CLI command.
* `main` reads commands and dispatches to the matching function.
* Uses `parson` for JSON parsing and construction.
* Uses helper functions from `requests.c`/`helpers.c` for building and sending HTTP requests.
* Validations and error handling are present before attempting network operations (e.g., checking login state).

---

## Implemented commands (description and implementation notes)

> Each command corresponds to one function in the code. The descriptions below summarize the implementation details.

### 1. `login_admin`

* Builds a `POST` request with the provided username and password using `compute_post_request`.
* If a valid cookie already exists (user already logged in), prints an error.
* Parses the response, checks for errors, updates the session cookie using `update_cookie`, and sets `is_admin = true` if successful.

### 2. `add_user`

* Admin-only command.
* Verifies an admin is logged in by checking the cookie.
* Reads the new user credentials, builds a JSON object, and sends a `POST` request to the corresponding endpoint.
* Parses the response and prints success or error messages.

### 3. `get_users`

* Admin-only command.
* Sends a `GET` request (including the session cookie) to retrieve the user list.
* Parses the JSON array of users and prints each user with an index, username, and password.

### 4. `delete_user`

* Reads the username to delete, builds the endpoint path (using `snprintf` to avoid buffer overflows) and sends a `DELETE` request.

### 5. `logout_admin`

* Sends a `GET` request to the logout endpoint.
* Releases the cookie and token, resets `is_admin` and `is_user` flags, and prints a message.
* Ensures an admin is logged in before attempting logout.

### 6. `login`

* Same approach as `login_admin` but for a normal user.
* Sets `is_user` instead of `is_admin` on successful login.
* Includes checks to prevent multiple simultaneous authentications.

### 7. `get_access`

* Obtains a JWT token for library access by sending a `GET` request.
* Parses the response to extract the token and saves it in the global `jwt_token` variable.
* Checks `is_user` (a normal logged-in user) rather than the cookie for eligibility.

### 8. `get_movies`

* Requires the user to be logged in and to have valid library access (JWT token).
* Sends a `GET` request to fetch all movies and parses the JSON response.
* Prints movie details and handles errors such as missing access or JSON parsing failures.

### 9. `delete_movie`

* Reads the movie ID, builds the request URL, and sends a `DELETE` request.
* Includes the `Authorization: Bearer <jwt_token>` header.
* Parses the response to determine success.

### 10. `get_movie`

* Reads the movie ID, sends a `GET` request for that movie.
* Uses Parson functions to extract title, year, genre, and other fields.
* For array fields, loops through the array and prints each element.

### 11. `add_movie`

* Reads all movie details from the user (title, year, genre, etc.) and builds a JSON object.
* Uses Parson to create and populate arrays for multi-valued fields.
* Sends a `POST` request with the JSON payload and validates input before sending.

### 12. `update_movie`

* Updates an existing movie using a `PUT` request. `compute_put_request` was implemented similarly to `compute_post_request`.
* Reads and processes input similar to `add_movie`, but includes the target movie ID.

### 13. `get_collections`

* Sends a `GET` request to retrieve all collections.
* Parses the response and prints each collection’s title and the IDs of its movies.

### 14. `get_collection`

* Reads a collection ID, sends a `GET` request, parses JSON, and prints the collection title and movie IDs.

### 15. `add_collection`

* Multi-step process:

  1. Create a new collection with a title.
  2. Validate provided movie IDs against the library using `exist_film`.
  3. Report invalid IDs and continue only if at least one valid film ID exists.
  4. After creating the collection, parse the response to obtain the new collection ID.
  5. Send separate `POST` requests to add each valid movie to the collection.

### 16. `delete_collection`

* Reads the collection ID and sends a `DELETE` request for that collection.
* Ensures the user has library access and validates operation success.

### 17. `add_movie_to_collection`

* Reads collection ID and movie ID, constructs the endpoint, and sends a `POST` with a JSON body containing the movie ID.
* Requires valid library access.

### 18. `delete_movie_from_collection`

* Constructs and sends a `DELETE` request that includes both the collection ID and the movie ID in the URL.

### 19. `logout`

* Logs out the current user by sending a `GET` request to the logout endpoint.
* Releases cookie and token, resets flags, and prints the result.

### 20. `update_cookie`

* Extracts and updates the session cookie from HTTP responses.
* Looks for the `Set-Cookie` header, extracts the cookie value, and stores it in the global `session_cookie` variable.
* Handles missing or invalid headers safely.

### 21. `exist_film`

* Helper function that checks whether a film with a given ID exists in the library.
* Sends a `GET` request for the film and returns `1` if it exists, `0` otherwise.
* Used during collection creation to validate film IDs.

---

## Notes and implementation details

* I used `snprintf` when building URLs or other strings that contain user-provided data to avoid buffer overflows.
* Parson is used both for constructing request payloads and parsing responses.
* The JWT token is attached to request headers where required (for library operations).
* All functions that perform network operations check the appropriate login/access flags before proceeding.
* Error handling is implemented for typical failure cases (missing access, malformed JSON responses, invalid IDs).

---

## Possible improvements / TODO

* Centralize error handling and response parsing to reduce duplicated code.
* Improve input validation and add more user-friendly prompts.
* Add unit tests for helper functions and for JSON construction/parsing.

---
