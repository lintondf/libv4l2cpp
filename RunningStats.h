#ifndef RUNNINGSTATS_H
#define RUNNINGSTATS_H
// https://www.johndcook.com/blog/skewness_kurtosis/
class RunningStats
{
public:
    RunningStats();
    void Clear();
    void Push(double x);
    long long NumDataValues() const;
    double Mean() const;
    double Variance() const;
    double StandardDeviation() const;
    double Skewness() const;
    double Kurtosis() const;
    double Max() const { return _max; };
    double Min() const { return _min; };

    friend RunningStats operator+(const RunningStats a, const RunningStats b);
    RunningStats& operator+=(const RunningStats &rhs);

private:
    long long n;
    double M1, M2, M3, M4;
    double _max, _min;
};

#endif
