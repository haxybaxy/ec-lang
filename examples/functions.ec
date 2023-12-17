print "=== Function 1: sayHi ===";
story sayHi(first, last) {
  print "Hi, " + first + " " + last + "!";
}
sayHi("Dear", "Reader");

print "=== Function 2: fib ===";
story fib(n) {
  if (n <= 1) return n;
  return fib(n - 2) + fib(n - 1);
}
for (my i = 0; i < 20; i = i + 1) {
  show fib(i);
}

show "=== Function 3: closure ===";
story makeCounter() {
  var i = 0;
  story count() {
    i = i + 1;
    show i;
  }

  return count;
}
my counter = makeCounter();
for (my j = 0; j < 10; j = j + 1) {
  counter();
}
