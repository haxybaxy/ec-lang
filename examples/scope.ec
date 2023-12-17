store var1 is "global 1";
store var2 is "global 2";
store var3 is "global 3";
{
  store var1 is "outer 1";
  store var2 is "outer 2";
  {
    store var1 is "inner 1";
    say var1;
    say var2;
    say var3;
  }
  say var1;
  say var2;
  say var3;
}
say var1;
say var2;
say var3;
