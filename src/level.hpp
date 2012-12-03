#ifndef LEVEL_HPP_
#define LEVEL_HPP_

#include "algebra.hpp"
//#include <list>
#include <vector>

enum SegmentType {
    BEZIER,
    STRAIGHT,
    T
};

const size_t nBezierSegments = 21;

class Segment {
public:
    virtual SegmentType getType() const = 0;
    Segment() : next (NULL), prev (NULL) {}
    virtual ~Segment() {}

    Segment* next; // can be NULL
    Segment* prev; // can be NULL

    virtual size_t num() const = 0;
    virtual Vector3D p(size_t i) const = 0;
    virtual Vector3D n(size_t i) const = 0;
    virtual Vector3D d(size_t i) const = 0;

    virtual Vector3D p(double t) const;
    virtual Vector3D n(double t) const;
    virtual Vector3D d(double t) const;
};

class TwistSegment : public Segment {
public:
    TwistSegment(double a) : a (a) {}
    double a; // amount of twist
};

class BezierSegment : public TwistSegment {
public:
    BezierSegment(const Vector3D curve[4], double a);
    SegmentType getType() const { return BEZIER; }

    size_t num() const { return nBezierSegments + 1; };
    Vector3D p(size_t i) const { return _p[i]; }
    Vector3D n(size_t i) const { return _n[i]; }
    Vector3D d(size_t i) const { return _d[i]; }

    Vector3D p(double t) const;
    //Vector3D n(double t) const;
    Vector3D d(double t) const;

private:
    void calcUntwisted();
    void calcTwist(double a_begin);

    Vector3D c[4];

    Vector3D _p[nBezierSegments+1];
    Vector3D _d[nBezierSegments+1];
    Vector3D _n[nBezierSegments+1];

    friend class Level;
};

class StraightSegment : public TwistSegment {
public:
    StraightSegment(const Vector3D ends[2], double a);
    SegmentType getType() const { return STRAIGHT; }

    size_t num() const { return nBezierSegments + 1; }
    Vector3D p(size_t i) const;
    Vector3D n(size_t i) const { return _n[i]; }
    Vector3D d(size_t i) const { return _d; }
private:
    void calc(const Vector3D& n_begin);

    Vector3D c[2];
    Vector3D _d;
    double len;
    Vector3D _n[nBezierSegments+1];

    friend class Level;
};

class TSegment : public Segment {
public:
    TSegment(const Vector3D ends[2]);
    SegmentType getType() const { return T; }

    size_t num() const { return 1; }
    Vector3D n(size_t i) const { return n_begin; }
    Vector3D p(size_t i) const { return c[i]; }
    Vector3D d(size_t i) const { return _d; }

    /*virtual size_t num() const { return 0; };
    virtual Vector3D p(size_t i) const;
    virtual Vector3D n(size_t i) const { return n_begin; }
    virtual Vector3D d(size_t i) const { return _d; }*/

    Segment* side[4]; // each can be null
private:
    void calc(const Vector3D& n_begin);

    Vector3D c[2];
    Vector3D _d;
    double len;
    Vector3D n_begin;

    friend class Level;
};

class Level {
public:
    std::vector<Segment*> segments;
    void calc();
private:
};

#endif
