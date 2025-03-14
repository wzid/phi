# spunc
 my first compiled language using llvm


## syntax
```
func get_name(): string => "spunc"

// it would be cool if I could remove the : i32 and just infer the return type
func add_five(a: int): int => a + 5

func main() {
    string name = get_name()
    int result = add_five(5)
    print(name)
    print(result)
}
```