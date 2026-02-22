# FluffOS XKTX Fork Changelog

## 2026-02-23

### PACKAGE_XK: 修复 json_parse 内存泄漏 (严重)

**文件**: `src/packages/xk/xk.cc`

**问题**: `json_parse__resize_array()` 函数在扩容 JSON 数组时，`allocate_array` + `memcpy` 后未释放旧数组，导致每次包含数组的 `json_parse` 调用都会泄漏内存。

由于所有 PostgreSQL 查询结果（通过 `pg_call`/`pg_get`）和角色存档都使用 JSON 格式，64 个在线玩家可导致 ~50MB/分钟 的内存增长，几小时内耗尽服务器内存。

**修复**:
```c
// 修复前（泄漏）:
void json_parse__resize_array(svalue_t *v, size_t m) {
  size_t n = v->u.arr->size;
  v->u.arr = allocate_array(m);          // 旧数组丢失！
  memcpy(v->u.arr->item, ...);
}

// 修复后:
void json_parse__resize_array(svalue_t *v, size_t m) {
  size_t n = v->u.arr->size;
  array_t *oldarr = v->u.arr;
  array_t *newarr = allocate_array(m);
  size_t copy_n = m < n ? m : n;
  memcpy(newarr->item, oldarr->item, copy_n * sizeof(svalue_t));
  // 浅拷贝后将旧数组中已拷贝的 items 清零，防止 free_array 时 double-free
  for (size_t i = 0; i < copy_n; i++) {
    oldarr->item[i] = const0u;
  }
  free_array(oldarr);
  v->u.arr = newarr;
}
```

**验证**: 10000 次 json_parse 调用测试 — 旧版泄漏 12,422KB (~1.27KB/次)，新版泄漏 0KB。生产环境 yitong 服务器(64人) 内存增长从 ~50MB/min 降为 0。

### PACKAGE_XK: 修复 Postgres_fetch_row NULL 字段处理

**文件**: `src/packages/xk/xk.cc`

**问题**: `Postgres_fetch_row` 中 `PQgetisnull` 检查后使用了 `return` 而非 `continue`，导致 NULL 字段后的所有字段被跳过。

**修复**: 将 `return` 改为 `continue`。
