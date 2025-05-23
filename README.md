# LL(1) Parser 

## Contributors
Ayaan Khan 
Minahil Ali

A fully‑featured **LL(1) top‑down parser** It consumes:

1. A context‑free grammar (CFG) file
2. A list of space‑separated input strings

and produces:

* FIRST/FOLLOW sets
* An LL(1) parsing table (pretty‑printed)
* A **step‑by‑step parsing trace** in `Stack | Input | Action` table format
* Per‑line verdicts (✅ success / ❌ syntax error)

All reports are streamed both to the console **and** to the user‑supplied output file.

---

## Building

### One‑liner (POSIX shell)

```bash
# build everything into a single executable named app
$ g++ -o app .\parseStack.cpp .\sourceCFG.cpp
```

---

## Running

```bash
$ ./app grammar.txt input.txt output.txt     
```

* **`grammar.txt`** Context‑free grammar (one production per line, `→` written as `->`).
* **`input.txt`**    Every non‑empty line is parsed as an independent string.
* **`output.txt`**  All console prints are duplicated here for easy submission.

> **Example:**
>
> ```bash
> $ ./app grammar.txt input.txt output.txt   
> ```

The console (and `output.txt`) will now show:

```
===== LL(1) Parsing Table =====
…
===== PARSING INPUT STRINGS =====
Code  : int x ;
+----------------------------------------+----------------------------------------+------------------------------+
| STACK                                  | INPUT                                  | ACTION                       |
+----------------------------------------+----------------------------------------+------------------------------+
| S $                                    | int id ; $                             | Initial state                |
| …                                      | …                                      | …                            |
+----------------------------------------+----------------------------------------+------------------------------+
Result: Line 1 parsed successfully.
```

---

## License

This code is provided for educational purposes under the MIT License.  See `LICENSE` for details.

