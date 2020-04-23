import unittest

# Run from the root dir with >python ./tests/run.py
# Remember to install the package first (>pip install . or
# >pip install -e .  for dev/edit mode))
if __name__ == "__main__":
    tests = unittest.defaultTestLoader.discover("./tests")
    runner = unittest.TextTestRunner()
    runner.run(tests)
