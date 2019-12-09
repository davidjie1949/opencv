#include "dbscan.h"
#include <math.h>

DBSCAN::DBSCAN(double epsilon, unsigned int minPoints)
    :m_epsilon(epsilon), m_minPoints(minPoints){
}

vector<int> DBSCAN::regionQuery(unsigned int index){
    vector<int> nb;
    for (int i = 0; i < this->m_dataset.size(); i++){
        double D = sqrt(pow((this->m_dataset[index].first - this->m_dataset[i].first), 2)
                        + pow((this->m_dataset[index].second - this->m_dataset[i].second), 2));
        if (D <= this->m_epsilon)
            nb.push_back(i);
    }
    return nb;
}

void DBSCAN::expandCluster (unsigned int index, vector<int> neighbors, int C){
    this->m_idx[index] = C;
    int k = 0;
    while (1){
        int j = neighbors[k];
        if (!this->m_visited[j]){
            this->m_visited[j] = 1;
            vector<int> neighbors2 = regionQuery(j);
            if (neighbors2.size() >= this->m_minPoints){
                neighbors.insert(neighbors.end(), neighbors2.begin(), neighbors2.end());
            }
        }
        if (this->m_idx[j] == -1) this->m_idx[j] = C;
        k++;
        if (k >= neighbors.size()) break;
    }
}

vector<int> DBSCAN::fit(const vector<pair<double, double>>& X){
    this->m_dataset = X;
    unsigned int n = X.size();
    for (unsigned int i = 0; i < n; i++){
        if(m_idx.size() <= i)
            this->m_idx.push_back(-1);
        else
            m_idx[i] = -1;

        if(m_visited.size() <= i)
            this->m_visited.push_back(0);
        else
            m_visited[i] = 0;

        if(m_isnoise.size() <= i)
            this->m_isnoise.push_back(0);
        else
            m_isnoise[i] = 0;
    }

    int C = -1;
    for(unsigned int i = 0; i < n; i++){
        if(!this->m_visited[i]){
            this->m_visited[i] = 1;
            vector<int> neighbors = regionQuery(i);
            if(neighbors.size() < this->m_minPoints)
                this->m_isnoise[i] = 1;
            else{
                C++;
                this->expandCluster(i, neighbors, C);
            }
        }
    }
    return this->m_idx;
}

void DBSCAN::setEpsilon(double epsilon){
    this->m_epsilon = epsilon;
}
void DBSCAN::setMinPoints(unsigned int minPoints){
    this->m_minPoints = minPoints;
}

double DBSCAN::getEpsilon(){
    return this->m_epsilon;
}
unsigned int DBSCAN::getMinPoints(){
    return this->m_minPoints;
}
