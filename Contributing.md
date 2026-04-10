
## Contributing code

If `clang` is installed on your platform, automatic format will be available:

```sh
# this format all files using clang-format
cd LampColorControler
make format
```

You can verify that all files are properly formatted with:

```sh
# this fails if some files are not properly formatted
cd LampColorControler
make format-verify
```

You can add this verification as a [git pre-commit
hook](https://git-scm.com/book/ms/v2/Customizing-Git-Git-Hooks) with:

```sh
# before every "git commit" checks if formatting is good
cd LampColorControler
make format-hook
git commit # will implictly run "make format-verify"
```

You can check that all files still build after your change using:

```sh
# clean then build everything, including all lamp type
cd LampColorControler
make verify-all
```

Note that simulator files are build only if all dependencies are available.
