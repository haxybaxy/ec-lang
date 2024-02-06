say "** For loop **";
store a = 0;
store temp = 0;
store b = 1;
for (b; a < 100; b = temp + b) {
  say a;
  temp = a;
  a = b;
}
