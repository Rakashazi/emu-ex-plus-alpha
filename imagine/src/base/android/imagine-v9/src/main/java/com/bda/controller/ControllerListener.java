package com.bda.controller;

public abstract interface ControllerListener
{
  public abstract void onKeyEvent(KeyEvent paramKeyEvent);

  public abstract void onMotionEvent(MotionEvent paramMotionEvent);

  public abstract void onStateEvent(StateEvent paramStateEvent);
}