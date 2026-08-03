# Parallel multi-seed example

This example prepares one immutable model, creates four independent `Local_MIP` instances with different random seeds, and runs them in caller-owned `std::thread`s. After all threads finish, it selects the best finite feasible objective using the original objective direction.

Every solver prints the same model address to show that variables, constraints, coefficients, presolve results, and derived model indexes are shared rather than copied. Each solver still owns its RNG, solution, tabu state, activities, callbacks, timer, and sampling buffers.

Build all examples from the `example/` directory:

```bash
./prepare.sh
./build.sh
./parallel-multiseed/parallel_multiseed_demo
```

Pass an `.mps` or `.lp` path as the first argument to use another model.

Important boundaries:

- One `Local_MIP` instance runs exactly one single-threaded search trajectory; create another solver to search again.
- The application, not Local-MIP, creates and joins the worker threads.
- Local-MIP has no internal thread pool or parallel solve API.
- The CLI remains a single-model, single-solver, single-search process.
- Solvers share only the frozen `Prepared_Model`; RNG and search state are independent, and incumbents are not synchronized during search.
- If multiple solvers use the same callback `user_data`, the application must synchronize that data.
- Python `run()` releases the GIL, but Python callbacks reacquire it and can serialize callback-heavy workloads.
