# Compatibility Shim Inventory

`registry/compatibility_shims.json` is the canonical inventory for temporary
compatibility shims. Each entry records its source file, symbol, owning roadmap
task, and removal condition. The inventory is currently empty because completed
migrations have no remaining production shim.

Any temporary shim must carry a matching source marker:

```c
/* YTNOVA_COMPAT_SHIM: id=example owner=51.2.2 removal=example-replaced */
```

`make qa-compatibility-shims` rejects untagged, unregistered, mismatched, and
completed-task shim markers.
