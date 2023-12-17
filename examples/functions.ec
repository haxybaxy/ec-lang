print "** Function 1:  sayJoke**";
story sayJoke(joke, punchline) {
  say (joke) + " " + (punchline);
}
sayJoke("Why doesn't C get respect from C++ and Java?", "It doesn't have class");


 "** Function 2: fib **";
action fib(n) {
  if (n <= 1) give n;
  give fib(n - 2) + fib(n - 1);
}
for (store i = 0; i < 20; i = i + 1) {
  say fib(i);
}

say "** Function 3: closure **";
action keepCount() {
  store i = 0;
  action count() {
    i = i + 1;
    show i;
  }

  return count;
}
store counter = keepCount();
for (store j = 0; j < 10; j = j + 1) {
  counter();
}

say "** Function 4: squaring **"
action square(x) {
    give x * x;
}

store a = 10;
store b = 20;
store result = square(a) + b * b - 100 / 5 % 8;
say(result);
