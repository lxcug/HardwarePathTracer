

## RenderGraph Usage

```c++
RenderGraph Graph;


RenderPassBase* Pass = new ();
Graph.AddPass(
    "PassName",
    Pass,
    []() {
        // 
    }
);
```

