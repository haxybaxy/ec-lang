print "** Function 1:  sayJoke**";
action sayJoke(joke, punchline) {
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
    say i;
  }

  return count;
}
store counter = keepCount();
for (store j = 0; j < 10; j = j + 1) {
  counter();
}
