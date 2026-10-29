# Terminator task-by-task shell test helpers

This bundle contains one `.sh` helper for each of the 11 tasks in the Terminator project PDF.

## Use

1. Extract this ZIP.
2. Open Terminal and `cd` into the extracted folder.
3. Run a helper, for example:

   ```bash
   ./test_task04_redirection.sh
   ```

4. Each helper creates safe test fixtures under `./terminator_test_fixtures/taskNN` (unless you pass a different directory).
5. Follow the printed steps inside your Terminator GUI and record pass/fail.

You can provide a custom fixture path:

```bash
./test_task04_redirection.sh "$HOME/terminator_test_fixtures/task04"
```

## Important limitations

- These are test-preparation helpers and manual test checklists, not automated end-to-end tests of the Terminator GUI.
- Most tests must be entered into Terminator itself because the project uses an X11 GUI and its own command parser.
- `test_task09_history.sh` generates `history_commands.txt`; it does NOT automatically submit 10,100 commands to Terminator.
- Task 11 helpers create script fixtures. They are intended to be run through Terminator to verify that your own interpreter processes them.
- Run potentially destructive tests (especially `exit`, Ctrl+D, and large history tests) in a disposable test setup.
- The project PDF specifies Linux. If you develop/test on macOS with XQuartz, some Linux-specific behaviors may differ; final validation should be on the target Linux environment if required by your course.
