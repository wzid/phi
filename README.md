# phi φ
A compiler written in C using LLVM

The name is inspired from [Euler's totient function](https://en.wikipedia.org/wiki/Euler%27s_totient_function) which is also called Euler's phi function.

Watch my devlogs on YouTube [here](https://www.youtube.com/watch?v=QPHYcFLAWoo&list=PLEgtx_e7NiZeAXGu8U04pLPKDU5zTNG4M&index=3)!


## syntax
```
func get_name(): string => "phi";

// it would be cool if I could remove the : int and just infer the return type
func add_five(a: int): int => a + 5;

func main() {
    string name = get_name();
    int result = add_five(5);
    print(name);
    print(result);
}
```
