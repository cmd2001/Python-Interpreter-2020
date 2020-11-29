//
// Created by Amagi_Yukisaki on 2019/11/18.
//

#ifndef PYTHON_INTERPRETER_DATATYPES_HPP
#define PYTHON_INTERPRETER_DATATYPES_HPP

#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <vector>
#include <set>
#include <cmath>
#ifndef debug
#define debug cerr
#endif
using namespace std;

inline int dcmp(const double &a, const double &b) {
    return fabs(a - b) <= 1e-10 ? 0 : a < b ? -1 : 1;
}

class BigInt {
private:
    vector<int> dat;
    bool isNeg; // isNeg will be zero for 0.
    inline int length() const {
        return dat.size();
    }
    inline int& operator [] (const int &x) {
        return dat[x];
    }
    inline const int operator [] (const int &x) const {
        return x < dat.size() ? dat[x] : 0;
    }
    inline BigInt octShl(int x) const {
        BigInt ret;
        ret.isNeg = isNeg;
        for(int i = 0; i < x; i++) ret.dat.push_back(0);
        for(int i = 0; i < dat.size(); i++) ret.dat.push_back(dat[i]);
        return ret;
    }
    friend bool cmpAbs(const BigInt &a, const BigInt &b) {
        if (a.length() != b.length()) return a.length() < b.length();
        for(int i = a.length() - 1; ~i; i--) if (a[i] != b[i]) return a[i] < b[i];
        return 1;
    }
public:
    BigInt() {
        isNeg = 0;
    }
    BigInt(int x) {
        isNeg = 0;
        while(x) dat.push_back(x % 10), x /= 10;
        reverse(dat.begin(), dat.end());
    }
    BigInt(const string &s) {
        isNeg = 0;
        for(int i = 0; i < s.length(); i++) dat.push_back(s[i] - '0');
        reverse(dat.begin(), dat.end());
        while(length() && !dat[length() - 1]) dat.resize(length() - 1);
        if(!length()) isNeg = 0;
    }
    friend bool operator == (const BigInt &a, const BigInt &b) {
        if(a.isNeg != b.isNeg || a.length() != b.length()) return 0;
        for(int i = 0; i < a.length(); i++) if(a[i] != b[i]) return 0;
        return 1;
    }
    friend bool operator != (const BigInt &a, const BigInt &b) {
        return ! (a == b);
    }
    friend bool operator < (const BigInt &a, const BigInt &b) {
        if(a.isNeg != b.isNeg) return a.isNeg;
        if(!a.isNeg) {
            if (a.length() != b.length()) return a.length() < b.length();
            for(int i = a.length() - 1; ~i; i--) if (a[i] != b[i]) return a[i] < b[i];
        } else {
            if(a.length() != b.length()) return a.length() > b.length();
            for(int i = a.length() - 1; ~i; i--) if(a[i] != b[i]) return a[i] > b[i];
        }
        return 0;
    }
    friend bool operator <= (const BigInt &a, const BigInt &b) {
        return a == b || a < b;
    }
    friend bool operator > (const BigInt &a, const BigInt &b) {
        return b < a;
    }
    friend bool operator >= (const BigInt &a, const BigInt &b) {
        return b <= a;
    }
    BigInt operator - () const {
        BigInt ret = *this;
        if(ret.length()) ret.isNeg ^= 1;
        return ret;
    }
    friend BigInt operator + (const BigInt &a, const BigInt &b) {
        BigInt ret;
        ret.isNeg = a.isNeg;
        ret.dat.resize(max(a.length(), b.length()) + 10);
        if(a.isNeg == b.isNeg) {
            for(int i = 0; i < max(a.length(), b.length()); i++) {
                ret[i] += a[i] + b[i];
                ret[i + 1] += ret[i] / 10, ret[i] %= 10;
            }
            while(ret.length() && !ret[ret.length() - 1]) ret.dat.resize(ret.length() - 1);
        } else {
            if(cmpAbs(b, a)) { // a >= b
                for(int i = 0; i < max(a.length(), b.length()) + 10; i++) {
                    ret[i] += a[i] - b[i];
                    if (ret[i] < 0) ret[i] += 10, ret[i + 1]--;
                }
                while(ret.length() && !ret[ret.length() - 1]) ret.dat.resize(ret.length() - 1);
            } else { // otherwise
                ret.isNeg = b.isNeg;
                for(int i = 0; i < max(a.length(), b.length()) + 10; i++) {
                    ret[i] += b[i] - a[i];
                    if (ret[i] < 0) ret[i] += 10, ret[i + 1]--;
                }
                while(ret.length() && !ret[ret.length() - 1]) ret.dat.resize(ret.length() - 1);
            }
        }
        if(!ret.length()) ret.isNeg = 0;
        return ret;
    }
    friend BigInt operator - (const BigInt &a, const BigInt &b) {
        return a + (-b);
    }
    friend BigInt operator * (const BigInt &a, const BigInt &b) {
        BigInt ret;
        ret.isNeg = a.isNeg ^ b.isNeg;
        ret.dat.resize(a.length() + b.length() + 10);
        for(int i = 0; i < a.length(); i++) for(int j = 0; j < b.length(); j++) ret[i + j] += a[i] * b[j];
        for(int i = 0; i < ret.length() - 1; i++) ret[i + 1] += ret[i] / 10, ret[i] %= 10;
        while(ret.length() && !ret[ret.length() - 1]) ret.dat.resize(ret.length() - 1);
        return ret;
    }
    friend BigInt operator / (BigInt a, BigInt b) {
        BigInt ret;
        ret.dat.resize(max(a.length(), b.length()) + 5);
        bool flag = a.isNeg ^ b.isNeg;
        ret.isNeg = a.isNeg = b.isNeg = 0;
        for(int i = a.length() - b.length() + 1; i >= 0; i--) {
            BigInt temp = b.octShl(i);
            while(temp <= a) a -= temp, ++ret[i];
        }
        while(ret.length() && !ret[ret.length() - 1]) ret.dat.resize(ret.length() - 1);
        if(flag) {
            ret.isNeg = 1;
            if(a.length()) --ret;
        }
        return ret;
    }
    friend BigInt operator % (const BigInt &a, const BigInt &b) {
        return a - b * (a / b);
    }
    friend BigInt operator += (BigInt &a, const BigInt &b) {
        return a = a + b;
    }
    friend BigInt operator -= (BigInt &a, const BigInt &b) {
        return a = a - b;
    }
    friend BigInt operator *= (BigInt &a, const BigInt &b) {
        return a = a * b;
    }
    BigInt operator ++() {
        return *this = *this + BigInt(1);
    }
    BigInt operator ++(int x) {
        BigInt ret = *this;
        return *this = *this + BigInt(1), ret;
    }
    BigInt operator --() {
        return *this = *this - BigInt(1);
    }
    BigInt operator -- (int x) {
        BigInt ret = *this;
        return *this = *this - BigInt(1), ret;
    }
    inline double toDouble() const {
        double ret = 0;
        for(int i = length() - 1; ~i; i--) ret = ret * 10 + dat[i];
        return isNeg ? -ret : ret;
    }
    inline void fromDouble(const double &xx) {
        long long x = xx;
        isNeg = 0, dat.clear();
        if(x < 0) isNeg = 1, x = -x;
        while(x) dat.push_back(x % 10), x /= 10;
        while(length() && !dat[length() - 1]) dat.resize(length() - 1);
        if(!length()) isNeg = 0;
    }
    inline string toString() const {
        if(!length()) return "0";
        string ret = isNeg ? "-" : "";
        for(int i = length() - 1; ~i; i--) ret = ret + (char)('0' + dat[i]);
        return ret;
    }
    inline void fromString(const string &s) {
        isNeg = 0, dat.clear();
        for(int i = 0; i < s.length(); i++) dat.push_back(s[i] - '0');
        reverse(dat.begin(), dat.end());
        while(length() && !dat[length() - 1]) dat.resize(length() - 1);
        if(!length()) isNeg = 0;
    }
    inline bool toBool() const {
        return (bool) length();
    }
    inline void fromBool(const bool &t) {
        isNeg = 0, dat.clear();
        if(t) dat.push_back(1);
    }
};

enum Type {Bool, Int, Float, String, None}; // from left to right.

class DataType {
private:
    Type getType() const {
        return tpe;
    }
    inline void getNext() {
        if(getType() == Bool) *this = toInt();
        else if(getType() == Int) *this = toFloat();
        // only String is bigger than Float, but this is solved in fixType;
    }
    friend inline void fixType(DataType &a, DataType &b) { // convert calculable type.
        if(a.getType() == b.getType()) return;
        while(a.getType() != b.getType()) a.getType() < b.getType() ? a.getNext() : b.getNext();
        if(a.getType() == Bool) a = a.toInt(), b = b.toInt();
    }
    friend inline void fixType2(DataType &a, DataType &b) { // for calc without bool
        fixType(a, b);
        if(a.getType() == Bool) a.getNext(), b.getNext();
    }
public:
    Type tpe;
    bool data_Bool;
    BigInt data_Int;
    double data_Float;
    string data_String;
    DataType() {data_Bool = 0, data_Float = 0;}
    explicit DataType(const Type &_tpe) {tpe = _tpe, data_Bool = 0, data_Float = 0;}
    explicit DataType(const bool &x) {tpe = Bool, data_Bool = x;}
    explicit DataType(const BigInt &x) {tpe = Int, data_Int = x;}
    explicit DataType(const double &x) {tpe = Float, data_Float = x;}
    explicit DataType(const string &x) {tpe = String, data_String = x;}
    inline DataType toInt() const {
        if(tpe == Int) return *this;
        DataType ret(Int);
        if(tpe == Bool) ret.data_Int.fromBool(data_Bool);
        else if(tpe == Float) ret.data_Int.fromDouble(data_Float);
        else if(tpe == String) ret.data_Int.fromString(data_String);
        return ret;
    }
    inline DataType toFloat() const {
        if(tpe == Float) return *this;
        DataType ret(Float);
        if(tpe == Bool) ret.data_Float = data_Bool;
        else ret.data_Float = data_Int.toDouble();
        return ret;
    }
    inline DataType toString() const {
        if(tpe == String) return *this;
        DataType ret(String);
        if(tpe == None) ret.data_String = "None";
        if(tpe == Bool) ret.data_String = data_Bool ? "True" : "False";
        if(tpe == Int) ret.data_String = data_Int.toString();
        if(tpe == Float) {
            static char buf[1010];
            sprintf(buf, "%0.6f", data_Float);
            ret.data_String = string(buf);
        }
        return ret;
    }
    inline DataType toBool() const {
        if (tpe == Bool) return *this;
        DataType ret(Bool);
        if (tpe == Int) ret.data_Bool = data_Int.toBool();
        else if (tpe == Float) ret.data_Bool = data_Float;
        else if (tpe == String) ret.data_Bool = data_String.length();
        return ret;
    }
    inline string toPrint() const {
        if(getType() == String) return data_String;
        if(getType() == Int) return data_Int.toString();
        if(getType() == Bool) return data_Bool ? "True" : "False";
        if(getType() == None) return "None";
        static char buf[1010];
        if(getType() == Float) {
            sprintf(buf, "%0.6f", data_Float);
            return (string) buf;
        }
    }
    friend DataType operator + (DataType a, DataType b) {
        fixType2(a, b);
        if(a.getType() == Int) return DataType(a.data_Int + b.data_Int);
        if(a.getType() == Float) return DataType(a.data_Float + b.data_Float);
        if(a.getType() == String) return DataType(a.data_String + b.data_String);
    }
    friend DataType operator - (DataType a, DataType b) {
        fixType2(a, b);
        if(a.getType() == Int) return DataType(a.data_Int - b.data_Int);
        if(a.getType() == Float) return DataType(a.data_Float - b.data_Float);
    }
    friend DataType operator * (DataType a, DataType b) {
        if(a.getType() == String) {
            string ret = "";
            for(BigInt i = 1, t = b.toInt().data_Int; i <= t; i++) ret = ret + a.data_String;
            return DataType(ret);
        }
        swap(a, b);
        if(a.getType() == String) {
            string ret = "";
            for(BigInt i = 1, t = b.toInt().data_Int; i <= t; i++) ret = ret + a.data_String;
            return DataType(ret);
        }
        fixType2(a, b);
        if(a.getType() == Int) return DataType(a.data_Int * b.data_Int);
        if(a.getType() == Float) return DataType(a.data_Float * b.data_Float);
    }
    friend DataType operator / (DataType a, DataType b) {
        fixType2(a, b);
        if(a.getType() == Int) a.getNext(), b.getNext();
        if(a.getType() == Float) return DataType(a.data_Float / b.data_Float);
    }
    friend DataType dualDiv(DataType a, DataType b) {
        if(a.getType() == Bool) a = a.toInt();
        if(b.getType() == Bool) b = b.toInt();
        return DataType(a.data_Int / b.data_Int);
    }
    friend DataType operator % (DataType a, DataType b) {
        if(a.getType() == Bool) a = a.toInt();
        if(b.getType() == Bool) b = b.toInt();
        return DataType(a.data_Int % b.data_Int);
    }
    friend bool operator == (DataType a, DataType b) {
        if (max(a.getType(), b.getType()) >= 3 && a.getType() != b.getType()) return 0;
        fixType(a, b);
        if (a.getType() == Bool) return a.data_Bool == b.data_Bool;
        if (a.getType() == Int) return a.data_Int == b.data_Int;
        if (a.getType() == Float) return a.data_Float == b.data_Float; // !dcmp(a.data_Float, b.data_Float);
        if (a.getType() == String) return a.data_String == b.data_String;
        if(a.getType() == None) return 1;
    }
    friend bool operator != (const DataType &a, const DataType &b) {
        return !(a == b);
    }
    friend bool operator < (DataType a, DataType b) {
        fixType2(a, b);
        if(a.getType() == Int) return a.data_Int < b.data_Int;
        if(a.getType() == Float) return a.data_Float < b.data_Float; // dcmp(a.data_Float, b.data_Float) < 0;
        if(a.getType() == String) return a.data_String < b.data_String;
    }
    friend bool operator <= (const DataType &a, const DataType &b) {
        return a == b || a < b;
    }
    friend bool operator > (const DataType &a, const DataType &b) {
        return b < a;
    }
    friend bool operator >= (const DataType &a, const DataType &b) {
        return a == b || a > b;
    }
    DataType operator - () const {
        if(getType() == Float) return DataType(-data_Float);
        return DataType(-toInt().data_Int);
    }
    friend bool operator && (const DataType &a, const DataType &b) { // sometimes b may not exist.
        return a.toBool().data_Bool && b.toBool().data_Bool;
    }
    friend bool operator || (const DataType &a, const DataType &b) {
        return a.toBool().data_Bool || b.toBool().data_Bool;
    }
    bool operator ! () const {
        return !toBool().data_Bool;
    }
    friend DataType operator += (DataType &a, const DataType &b) {
        return a = a + b;
    }
    friend DataType operator -= (DataType &a, const DataType &b) {
        return a = a - b;
    }
    friend DataType operator *= (DataType &a, const DataType &b) {
        return a = a * b;
    }
    friend DataType operator /= (DataType &a, const DataType &b) {
        return a = a / b;
    }
    friend DataType dualDivEqual (DataType &a, const DataType &b) {
        return a = dualDiv(a, b);
    }
    friend DataType operator %= (DataType &a, const DataType &b) {
        return a = a % b;
    }
};

template <typename T>
class memPool {
private:
    set<T*> dat;
public:
    inline void push(T* x) {
        dat.insert(x);
    }
    ~memPool() {
        for(auto t: dat) delete t;
    }
};

memPool<DataType> VariableDataPool;

class Variable {
private:
    DataType* dst;
public:
    Variable() {
        dst = nullptr;
    }
    inline DataType& getContent() const {
        return *dst;
    }
    Variable operator = (const DataType &x) { // otherwise universal variables will not be modified.
        if(!dst) {
            dst = new DataType(x);
            VariableDataPool.push(dst);
        } else *dst = x;
        return *this;
    }
};

class VariableStack {
private:
    static constexpr int maxDep = 2e4 + 1e2;
    map<string, Variable> stk[maxDep];
    int top;
public:
    VariableStack() {
        top = 0;
    }
    inline void pop() {
        --top;
    }
    inline void push(bool type) { // type = 0 then build from 0, else build from top
        if(type) ++top, stk[top] = stk[top - 1];
        else stk[++top] = stk[0];
    }
    inline void merge(const map<string, Variable> &args) {
        for(auto t: args) stk[top][t.first] = t.second;
    }
    Variable& operator [] (const string &nme) {
        return stk[top][nme];
    }
};

enum Statement{Running, Continued, Broken, Returned};

class FlowStack {
private:
    static constexpr int maxDep = 2e4 + 1e2;
    Statement stk[maxDep];
    int top;
public:
    FlowStack() {
        memset(this, 0, sizeof *this);
    }
    inline void push(Statement x) {
        stk[++top] = x;
    }
    inline Statement query() {
        return stk[top];
    }
    inline void set(Statement x) {
        stk[top] = x;
    }
    inline void reset() {
        stk[top] = Running;
    }
    inline void pop() {
        --top;
    }
};



#endif //PYTHON_INTERPRETER_DATATYPES_HPP