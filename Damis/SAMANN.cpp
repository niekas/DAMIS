///////////////////////////////////////////////////////////
//  SAMANN.cpp
//  Implementation of the Class SAMANN
//  Created on:      07-Lie-2013 20:07:31
//  Original author: Povilas
///////////////////////////////////////////////////////////

#include "SAMANN.h"
#include "Statistics.h"
#include "DistanceMetrics.h"
#include "math.h"
#include "AdditionalMethods.h"
#include <fstream>
#include <iomanip>
#include <float.h>

SAMANN::SAMANN(){

}

SAMANN::~SAMANN(){

}

/**
 * Constructor that takes, no of learning elements, no of neurons in hidden layer,
 * learning speed value, and number of max training iterations.
 */
SAMANN::SAMANN(int m1, int nl, double e, int maxIter){
    mTrain = m1;
    nNeurons = nl;
    eta = e;
    maxIteration = maxIter;
    d = 2;
    initializeProjectionMatrix();
    stressErrors.reserve(maxIter);
}

/**
 * Calculates Lambda value for SAMAN projection
 */
double SAMANN::getLambda(){

    double temp = 0.0;
    int m = X.getObjectCount();
    for (int i = 0; i < m - 1; i++)
    {
        for (int j = i + 1; j < m; j++)
            temp = temp + DistanceMetrics::getDistance(X.getObjectAt(i), X.getObjectAt(j), EUCLIDEAN);
    }
    return 1.0 / temp;
}


/**
 * Pure virtual method that calculates the projection
 */
ObjectMatrix SAMANN::getProjection(){

    int n = X.getObjectAt(0).getFeatureCount();
    int m = X.getObjectCount();
    double ddelta_L[mTrain][d];
    double delta_tarp[mTrain];
    double ddelta_tarp[mTrain];
    double tarp = 0.0, tarp2 = 0.0, lambda, tmp, distXp, distYis;
    initializeWeights();   // w1, w2
    initializeExitMatrixes();   // Y_pasl, Y_is
    initializeDeltaL();   // delta_L
    std::ofstream file ("debug.txt");
        
    NormalizeX(); 
    
    //for (int i = 0; i < m; i++)
    //    X.updateDataObject(i, 0, 1.0);
    
    //getXp();
    //addObjectOfOnes();
    
    lambda = getLambda();
    file << lambda << std::endl;
    for (int i = 0; i < mTrain + 1; i++)
    {
        for (int j = 0; j < d + 1; j++)
            ddelta_L[i][j] = 0.0;
        delta_tarp[i] = 0.0;
        ddelta_tarp[i] = 0.0;
    }

    for (int niu = 1; niu < m + 1; niu++)
    {
        for (int j = 0; j < nNeurons; j++)
        {
            tarp2 = 0.0;
            for (int k = 0; k < n + 1; k++)
                tarp2 += w1[j].at(k) * X.getObjectAt(niu).getFeatureAt(k);
            Y_pasl.updateDataObject(niu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
        }
                
        for (int j = 0; j < d; j++)
        {
            tarp2 = 0.0;
            for (int k = 0; k < nNeurons + 1; k++)
                tarp2 += w2[j].at(k) * Y_pasl.getObjectAt(niu).getFeatureAt(k);
            Y_is.updateDataObject(niu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
        }
    }
    
    for (int iter = 0; iter < maxIteration; iter++)
    {
        getXp();
        for (int miu = 1; miu < mTrain; miu++)
        {
            for (int niu = miu + 1; niu < mTrain + 1; niu++)
            {                
                
                for (int j = 0; j < nNeurons; j++)
                {
                    tarp = 0.0;
                    tarp2 = 0.0;
                    for (int k = 0; k < n + 1; k++)
                    {
                        tarp += w1[j].at(k) * Xp.getObjectAt(miu).getFeatureAt(k);
                        tarp2 += w1[j].at(k) * Xp.getObjectAt(niu).getFeatureAt(k);
                        
                    }
                    
                    Y_pasl.updateDataObject(miu, j + 1, 1.0 / (1 + exp(-1 * tarp)));
                    Y_pasl.updateDataObject(niu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
                }
                
                for (int j = 0; j < d; j++)
                {
                    tarp = 0.0;
                    tarp2 = 0.0;
                    for (int k = 0; k < nNeurons + 1; k++)
                    {
                        tarp += w2[j].at(k) * Y_pasl.getObjectAt(miu).getFeatureAt(k);
                        tarp2 += w2[j].at(k) * Y_pasl.getObjectAt(niu).getFeatureAt(k);                        
                    }

                    Y_is.updateDataObject(miu, j + 1, 1.0 / (1 + exp(-1 * tarp)));
                    Y_is.updateDataObject(niu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
                }                
                
                distXp = DistanceMetrics::getDistance(Xp.getObjectAt(miu), Xp.getObjectAt(niu), EUCLIDEAN);
                distYis = DistanceMetrics::getDistance(Y_is.getObjectAt(miu), Y_is.getObjectAt(niu), EUCLIDEAN);
                if (distXp == 0)
                    distXp = 0.001;

                for (int k = 1; k < d + 1; k++)
                {            
                    tmp = -2 * lambda * ((distXp - distYis) / (distXp * distYis)) * (Y_is.getObjectAt(miu).getFeatureAt(k) - Y_is.getObjectAt(niu).getFeatureAt(k));
                    delta_L.updateDataObject(miu, niu, tmp);
                    ddelta_L[miu][k] = delta_L.getObjectAt(miu).getFeatureAt(niu) * (1 - Y_is.getObjectAt(miu).getFeatureAt(k)) * Y_is.getObjectAt(miu).getFeatureAt(k);
                    ddelta_L[niu][k] = delta_L.getObjectAt(miu).getFeatureAt(niu) * (1 - Y_is.getObjectAt(niu).getFeatureAt(k)) * Y_is.getObjectAt(niu).getFeatureAt(k);

                    for (int j = 0; j < nNeurons + 1; j++)
                    {
                        if (iter == 0)
                            w2[k][j] = -1 * eta * (ddelta_L[miu][k] * Y_pasl.getObjectAt(miu).getFeatureAt(j) - ddelta_L[niu][k] * Y_pasl.getObjectAt(niu).getFeatureAt(j));
                        else
                            w2[k][j] = -1 * eta * (ddelta_L[miu][k] * Y_pasl.getObjectAt(miu).getFeatureAt(j) - ddelta_L[niu][k] * Y_pasl.getObjectAt(niu).getFeatureAt(j)) + w2[k][j];
                    }
                }

                for (int j = 1; j < nNeurons + 1; j++)
                {
                    delta_tarp[miu] = 0;
                    delta_tarp[niu] = 0;
                    
                    for (int k = 1; k < d + 1; k++)
                    {
                        delta_tarp[miu] += ddelta_L[miu][k] * w2[k][j];
                        delta_tarp[niu] += ddelta_L[niu][k] * w2[k][j];
                    }
                    
                    ddelta_tarp[miu] = delta_tarp[miu] * (1 - Y_pasl.getObjectAt(miu).getFeatureAt(j)) * Y_pasl.getObjectAt(miu).getFeatureAt(j);
                    ddelta_tarp[niu] = delta_tarp[niu] * (1 - Y_pasl.getObjectAt(niu).getFeatureAt(j)) * Y_pasl.getObjectAt(niu).getFeatureAt(j);
                    
                    for (int k = 0; k < n + 1; k++)
                    {                        
                        if (iter == 0)
                            w1[j][k] = -1 * eta * (ddelta_tarp[miu] * Xp.getObjectAt(miu).getFeatureAt(k) - ddelta_tarp[niu] * Xp.getObjectAt(niu).getFeatureAt(k));
                        else
                            w1[j][k] = -1 * eta * (ddelta_tarp[miu] * Xp.getObjectAt(miu).getFeatureAt(k) - ddelta_tarp[niu] * Xp.getObjectAt(niu).getFeatureAt(k)) + w1[j][k];
                    }                       
                }
                
                for (int niuu = 1; niuu < m + 1; niuu++)
                {
                    for (int j = 0; j < nNeurons; j++)
                    {
                        tarp = 0.0;
                        for (int k = 0; k < n + 1; k++)
                            tarp2 += w1[j].at(k) * Xp.getObjectAt(niuu).getFeatureAt(k);                 
                        Y_pasl.updateDataObject(niuu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
                    }
                
                    for (int j = 0; j < d; j++)
                    {
                        tarp2 = 0.0;
                        for (int k = 0; k < nNeurons + 1; k++)
                            tarp2 += w2[j].at(k) * Y_pasl.getObjectAt(niuu).getFeatureAt(k);
                        Y_is.updateDataObject(niuu, j + 1, 1.0 / (1 + exp(-1 * tarp2)));
                    }
                }
            }             
        }
        stressErrors.push_back(getStress());
    }  // iteraciju pabaiga
    
    file.close();
    for (int miu = 1; miu < m + 1; miu++)
    {
        for (int j = 1; j < nNeurons + 1; j++)
        {
            tarp = 0.0;
            for (int k = 0; k < n + 1; k++)
                tarp += w1[j].at(k) * X.getObjectAt(miu).getFeatureAt(k);
            Y_pasl.updateDataObject(miu, j, 1.0 / (1 + exp(-1 * tarp)));              
        }
        for (int j = 1; j < d + 1; j++)
        {
            tarp = 0.0;
            for (int k = 0; k < nNeurons + 1; k++)
                tarp += w2[j].at(k) * Y_pasl.getObjectAt(miu).getFeatureAt(k);
            Y_is.updateDataObject(miu, j, 1.0 / (1 + exp(-1 * tarp)));
        }
    }
        
    return  Y_is;
}

double SAMANN::getMax()
{
    int m = X.getObjectCount();
    double max = DBL_MIN, dist;
    /*
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            if (max < X.getObjectAt(i).getFeatureAt(j))
                max = X.getObjectAt(i).getFeatureAt(j);
       */ 
    for (int i = 0; i < m - 1; i++)
    {
        for (int j = i + 1; j < m; j++)
        {
            dist = DistanceMetrics::getDistance(X.getObjectAt(i), X.getObjectAt(j), EUCLIDEAN);
            if (dist > max)
                max = dist;
        }
    }
    
    return max; 
}

void SAMANN::NormalizeX()
{
    double max = getMax();
    double value;
    int m = X.getObjectCount();
    int n = X.getObjectAt(0).getFeatureCount();
    std::vector<double> Xrow;
    Xrow.reserve(n + 1);
    ObjectMatrix Xtmp(m + 1);
    
    for (int i = 0; i < n + 1; i++)
        Xrow.push_back(1.0);
    Xtmp.addObject(DataObject(Xrow));
    Xrow.clear();
    for (int i = 0; i < m; i++)
    {
        Xrow.push_back(1.0);
        for (int j = 0; j < n; j++)
        {
            value = X.getObjectAt(i).getFeatureAt(j) / max;
            Xrow.push_back(value);
        }
        Xtmp.addObject(DataObject(Xrow));
        Xrow.clear();
    }
    X = Xtmp;
}



void SAMANN::getXp()
{
    int m = X.getObjectCount(), i = 0, k = 0, index = 0;
    Xp = ObjectMatrix(m);
    double r = Statistics::getRandom(1, m, k);
    DataObject dObject; 
    index = static_cast<int>(r);
    
    while (i < m)
    {       
        dObject = X.getObjectAt(index);
        if (isIdentical(dObject) == false)
        {
            Xp.addObject(dObject);
            i++;
        }
        
        index = (index + 1) % m;
        k++;
        
        if (k >= m && i < m)
        {
            dObject = X.getObjectAt(index);
            Xp.addObject(dObject);
            i++;
        }
    }
    
}

bool SAMANN::isIdentical(DataObject obj)
{
    bool ats = false;
    int n = Xp.getObjectCount();
    int m = obj.getFeatureCount();
    int k = 0;
    
    for (int i = 0; i < n; i++)
    {
        k = 0;
        for (int j = 0; j < m; j++)
            if (obj.getFeatureAt(j) == Xp.getObjectAt(i).getFeatureAt(j))
                k++;
        if (k == m)
        {
            ats = true;
            break;
        }
    }    
    return ats;
}

/**
 * calculates SAMAN stress
 */
double SAMANN::getStress(){
    double tarp1 = 0.0, tarp2 = 0.0, distX, distY;
    int m = X.getObjectCount();
    for (int miu = 1; miu < m - 1; miu++)
    {
        for (int niu = miu + 1; niu < m; niu++)
        {
            distX = DistanceMetrics::getDistance(X.getObjectAt(miu), X.getObjectAt(niu), EUCLIDEAN);
            distY = DistanceMetrics::getDistance(Y_is.getObjectAt(miu), Y_is.getObjectAt(niu), EUCLIDEAN);
            tarp1 += 1 / distX;
            tarp2 += pow(distX - distY, 2) / distX;
        }
    }
    return tarp1 * tarp2;
}


/**
 * Function forms set of training objects (uses Preprocess class function)
 */
void SAMANN::getTrainingSet(){

}


/**
 * Public function trains SAMAN network
 */
void SAMANN::train(){

}


/**
 * Function updates last and hidden layer neuron weights
 */
void SAMANN::updateWeights(){

}

void SAMANN::initializeWeights()
{
    int n = X.getObjectAt(0).getFeatureCount();
    std::vector<double> w1Row, w2Row;
    w1Row.reserve(n + 1);
    w2Row.reserve(nNeurons + 1);
    
    for (int i = 0; i < nNeurons + 1; i++)
    {
        for (int j = 0; j < n + 1; j++)
            w1Row.push_back(Statistics::getRandom(-0.1, 0.1, (i + 5 * j)));
        w1.push_back(w1Row);
        w1Row.clear();
    }
    
    for (int i = 0; i < d + 1; i++)
    {
        for (int j = 0; j < nNeurons + 1; j++)
            w2Row.push_back(Statistics::getRandom(-0.1, 0.1, (i + 5 * j)));
        w2.push_back(w2Row);
        w2Row.clear();
    }
}

void SAMANN::initializeExitMatrixes()
{
    int m = X.getObjectCount();
    Y_is = ObjectMatrix(m + 1);
    Y_pasl = ObjectMatrix(m + 1);
    std::vector<double> YisRow, YpaslRow;
    YisRow.reserve(d + 1);
    YpaslRow.reserve(nNeurons + 1);
       
    for (int i = 0; i < m + 1; i++)
    {
        for (int j = 0; j < d + 1; j++)
            YisRow.push_back(1.0);
        Y_is.addObject(DataObject(YisRow));
        
        for (int j = 0; j < nNeurons + 1; j++)
            YpaslRow.push_back(1.0);
        Y_pasl.addObject(DataObject(YpaslRow));   
        
        YisRow.clear();
        YpaslRow.clear();
    }   
}

void SAMANN::initializeDeltaL()
{
    delta_L = ObjectMatrix(mTrain + 1);

    std::vector<double> deltaLRow;
    deltaLRow.reserve(mTrain + 1);
    
    for (int i = 0; i < mTrain + 1; i++)
    {
        for (int j = 0; j < mTrain + 1; j++)
            deltaLRow.push_back(0.1);
        delta_L.addObject(DataObject(deltaLRow));        
        deltaLRow.clear();
    }   
}

void SAMANN::addObjectOfOnes()
{
    int m = X.getObjectCount();
    int n = X.getObjectAt(0).getFeatureCount();
    ObjectMatrix Xtmp(m + 1);
    std::vector<double> XtmpObject;
    XtmpObject.reserve(n);
    for (int i = 0; i < n; i++)
        XtmpObject.push_back(1.0);
    Xtmp.addObject(DataObject(XtmpObject));
    XtmpObject.clear();

    for (int i = 0; i < m; i++)
        Xtmp.addObject(X.getObjectAt(i));    
    X = Xtmp;
    /*
    m = Xp.getObjectCount();
    n = Xp.getObjectAt(0).getFeatureCount();
    
    XtmpObject.reserve(n);
    for (int i = 0; i < n; i++)
        XtmpObject.push_back(1.0);
    Xtmp = ObjectMatrix(m + 1);
    Xtmp.addObject(DataObject(XtmpObject));
    for (int i = 0; i < m; i++)
        Xtmp.addObject(Xp.getObjectAt(i));    
    Xp = Xtmp;
    */
}

std::vector<double> SAMANN::getStressErrors()
{
    return stressErrors;
}