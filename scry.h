#ifndef __SCRY_H__
#define __SCRY_H__

class MyClass
{
public:
  MyClass();

  /* use virtual otherwise linker will try to perform static linkage */
  virtual void DoSomething();

private:
  int x;
};

#endif
