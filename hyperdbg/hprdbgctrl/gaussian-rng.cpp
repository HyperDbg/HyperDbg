/**
 * @file GaussianRng.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Main interface to connect applications to driver
 * @details
 * @version 0.1
 * @date 2020-07-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief get the median of a vector
 * 
 * @param Cases all the elements
 * @return double median of elements
 */
double
Median(vector<double> Cases)
{
    size_t Size = Cases.size();

    if (Size == 0)
    {
        return 0; // Undefined, really
    }
    else
    {
        sort(Cases.begin(), Cases.end());
        if (Size % 2 == 0)
        {
            return (Cases[Size / 2 - 1] + Cases[Size / 2]) / 2;
        }
        else
        {
            return Cases[Size / 2];
        }
    }
}

/**
 * @brief get the average of a vector
 * 
 * @tparam T type of vector
 * @param vec all the elements
 * @return T the average of elements
 */
template <typename T>
T
Average(const vector<T> & vec)
{
    size_t Sz;
    T      Mean;
    Sz = vec.size();
    if (Sz == 1)
        return 0.0;

    //
    // Calculate the mean
    //
    Mean = std::accumulate(vec.begin(), vec.end(), 0.0) / Sz;

    return Mean;
}

/**
 * @brief get the standard deviation of elements
 * 
 * @tparam T type of vector
 * @param v all the elements
 * @return T the standard deviation of elements
 */
template <typename T>
T
CalculateStandardDeviation(const std::vector<T> & v)
{
    double Sum, Mean, SqSum, Stdev;

    Sum  = std::accumulate(v.begin(), v.end(), 0.0);
    Mean = Sum / v.size();

    SqSum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    Stdev = std::sqrt(SqSum / v.size() - Mean * Mean);
    return Stdev;
}

/**
 * @brief get the Median Absolute Deviation (MAD) Test
 * 
 * @param Data all the elements
 * @return double result of MAD test
 */
double
MedianAbsoluteDeviationTest(vector<double> Data)
{
    double MedianData;
    double Mad;

    MedianData = Median(Data);

    for (int i = 0; i < Data.size(); i++)
    {
        Data[i] = abs(Data[i] - MedianData);
    }
    Mad = 1.4826 * Median(Data);

    return Mad;
}

/**
 * @brief random generator based on calculations
 * 
 * @param mu 
 * @param sigma 
 * @return double random number in the range of gaussian curve
 */
double
Randn(double mu, double sigma)
{
    double        U1, U2, W, mult;
    static double X1, X2;
    static int    call = 0;

    if (call == 1)
    {
        call = !call;
        return (mu + sigma * (double)X2);
    }

    do
    {
        U1 = -1 + ((double)rand() / RAND_MAX) * 2;
        U2 = -1 + ((double)rand() / RAND_MAX) * 2;
        W  = pow(U1, 2) + pow(U2, 2);
    } while (W >= 1 || W == 0);

    mult = sqrt((-2 * log(W)) / W);
    X1   = U1 * mult;
    X2   = U2 * mult;

    call = !call;

    return (mu + sigma * (double)X1);
}

/**
 * @brief Calculate and generate random gaussian number
 * 
 * @param Data 
 * @param AverageOfData 
 * @param StandardDeviationOfData 
 * @param MedianOfData 
 */
VOID
GuassianGenerateRandom(vector<double> Data, UINT64 * AverageOfData, UINT64 * StandardDeviationOfData, UINT64 * MedianOfData)
{
    vector<double> FinalData;
    int            CountOfOutliers = 0;
    double         Medians;
    double         Mad;
    double         StandardDeviation;
    double         DataAverage;
    double         DataMedian;

    vector<double> OriginalData  = Data;
    vector<double> ChangableData = Data;

    Mad     = MedianAbsoluteDeviationTest(ChangableData);
    Medians = Median(OriginalData);

    for (auto item : OriginalData)
    {
        if (item > (3 * Mad) + Medians || item < -(3 * Mad) + Medians)
        {
            CountOfOutliers++;
        }
        else
        {
            FinalData.push_back(item);
        }
    }

    StandardDeviation = CalculateStandardDeviation(FinalData);
    DataAverage       = Average(FinalData);
    DataMedian        = Median(FinalData);

    //
    // Set the values to return
    //
    *AverageOfData = (UINT64)DataAverage;

    //
    // We add 5 to the standard deviation because this value might be
    // 0 or 1 so we need more variance
    //
    *StandardDeviationOfData = (UINT64)StandardDeviation + 5;
    *MedianOfData            = (UINT64)DataMedian;

    //
    // ShowMessages("Varience : %f\n", StandardDeviation);
    // ShowMessages("Mean : %f\n", DataAverage);
    // ShowMessages("Count of outliers : %d\n", CountOfOutliers);
    //
    //
    // for (int i = 0; i < 10000; i++)
    // {
    // 	ShowMessages("Final Random Time Stamp : %d\n", (int) Randn(DataAverage,
    // StandardDeviation));
    // _getch();
    // }
    //
}

/**
 * @brief A simple test for the data based on 
 * pre-defined numbers in a file
 * 
 * @return VOID 
 */
VOID
TestGaussianFromFile()
{
    vector<double> MyVector;
    UINT64         AverageOfData;
    UINT64         StandardDeviationOfData;
    UINT64         MedianOfData;

    std::ifstream file("C:\\Users\\sina\\Desktop\\r.txt");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            MyVector.push_back(stod(line.c_str()));
        }
        file.close();

        GuassianGenerateRandom(MyVector, &AverageOfData, &StandardDeviationOfData, &MedianOfData);
    }
}
