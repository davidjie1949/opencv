#ifndef DBSCAN_H
#define DBSCAN_H

#include <iostream>
#include <vector>

using namespace std;

class DBSCAN{
public:
    DBSCAN(double epsilon, unsigned int minPoints);
    ~DBSCAN();

    vector<int> fit(const vector<pair<double, double>>& X);

    double getEpsilon();
    unsigned int getMinPoints();

    void setEpsilon(double epsilon);
    void setMinPoints(unsigned int minPoints);

private:
    double m_epsilon;
    vector<pair<double, double>> m_dataset;
    vector<int> m_idx, m_visited, m_isnoise;
    unsigned int m_minPoints;
    vector<int> regionQuery(unsigned int index);
    void expandCluster(unsigned int index, vector<int> neighbors, int C);
};

#endif // DBSCAN_H
