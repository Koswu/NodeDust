#ifndef MYQUEUE_H_
#define MYQUEUE_H
class MyQueue {
  private:
    int _size;
    int _capacity;
    int _rear;
    int _front;
    double *_arr;
  public:
    MyQueue(int capacity) {
      _capacity = capacity;
      _size = 0;
      _rear = 0;
      _front = 0;
      _arr = new double[capacity];
      for (int i = 0; i < _capacity; i++)
      {
        _arr[i] = 0;
      }
    }
    const double* getArr() {
      return _arr;
    }
    int getRear() {
      return _rear;
    }
    int getFront() {
      return _front;
    }
    int size() {
      return _size;
    }
    int capacity() {
      return _capacity;
    }
    bool empty() {
      return _size == 0;
    }
    bool full() {
      return _size >= _capacity;
    }
    double front() {
      if (_size == 0)
      {
        return -1;
      }
      return _arr[_front];
    }
    double rear() {
      if (_size == 0)
      {
        return -1;
      }
      int index = _rear - 1;
      if (index < 0)
      {
          index += _capacity;                                                                                           
      }
      return _arr[index];
    }
    void pop() {
      if (_size == 0)
      {
        return;
      }
      _front++;
      if (_front >= _capacity)
      {
        _front -= _capacity;
      }
      _size--;
    }
    void push(double val) {
      if (++_size >= _capacity)
      {
        pop();
      }
      _arr[_rear] = val;
      _rear++;
      if (_rear >= _capacity)
      {
        _rear -= _capacity;
      }
    }
};
#endif 
