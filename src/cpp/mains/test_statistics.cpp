#include <tw/common_trade/Defs.h>
#include <tw/common/stats.h>

#include <stdio.h>
#include <math.h>

class StdDeviation
{
private:
    int max;
    double value[100];
    double mean;

public:
    double CalculateMean()
    {
        double sum = 0;
        for(int i = 0; i < max; i++)
            sum += value[i];
        return (sum / max);
    }

    double CalculateVariane()
    {
        mean = CalculateMean();

        double temp = 0;
        for(int i = 0; i < max; i++)
        {
             temp += (value[i] - mean) * (value[i] - mean) ;
        }

        return temp / max;
    }

    double CalculateSampleVariane()
    {
        mean = CalculateMean();

        double temp = 0;
        for(int i = 0; i < max; i++)
        {
             temp += (value[i] - mean) * (value[i] - mean) ;
        }

        return temp / (max - 1);
    }

    int SetValues(double *p, int count)
    {
        if(count > 100)
            return -1;

        max = count;
        for(int i = 0; i < count; i++)
            value[i] = p[i];

        return 0;
    }

    double Calculate_StandardDeviation()
    {
        return sqrt(CalculateVariane());
    }

    double Calculate_SampleStandardDeviation()
    {
        return sqrt(CalculateSampleVariane());
    }
};

class FinanceCalculator
{
public:
    double XSeries[100];
    double YSeries[100];
    int max;

    StdDeviation x;
    StdDeviation y;

public:
    void SetValues(double *xvalues, double *yvalues, int count)
    {
        for(int i = 0; i < count; i++)
        {
            XSeries[i] = xvalues[i];
            YSeries[i] = yvalues[i];
        }

        x.SetValues(xvalues, count);
        y.SetValues(yvalues, count);
        max = count;
    }

    double Calculate_Covariance()
    {
        double xmean = x.CalculateMean();
        double ymean = y.CalculateMean();

        double total = 0;
        for(int i = 0; i < max; i++)
        {
            total += (XSeries[i] - xmean) * (YSeries[i] - ymean);
        }

        return total / max;
    }

    double Calculate_Correlation()
    {
        double cov = Calculate_Covariance();
        double correlation = cov / (x.Calculate_StandardDeviation() * y.Calculate_StandardDeviation());
        return correlation;
    }
};

void setValues(double *xvalues, double *yvalues, int count, FinCalc& f) {
    for(int i = 0; i < count; i++) {
        f.getX().getValues().push_back(xvalues[i]);
        f.getY().getValues().push_back(yvalues[i]);
    }
}

int main()
{
    FinanceCalculator calc;
    
    {
        printf("\n\nZero Correlation and Covariance Data Set\n");
        double xarr[] = { 8, 6, 4, 6, 8 };
        double yarr[] = { 10, 12, 14, 16, 18 };

        calc.SetValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]));

        printf("Mean x = %.10lf\n", calc.x.CalculateMean());
        printf("Mean y = %.10lf\n", calc.y.CalculateMean());
        printf("Var x = %.10lf\n", calc.x.CalculateVariane());
        printf("Var y = %.10lf\n", calc.y.CalculateVariane());
        printf("StdDev x = %.10lf\n", calc.x.Calculate_StandardDeviation());
        printf("StdDev y = %.10lf\n", calc.y.Calculate_StandardDeviation());
        printf("Covariance = %.10lf\n", calc.Calculate_Covariance());
        printf("Correlation = %.10lf\n", calc.Calculate_Correlation());
        
        printf("\n=====================\n");
        
        FinCalc f;
        setValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]), f);
        
        printf("Min x (NEW) = %.10lf\n", f.getX().calcMin());
        printf("Max x (NEW) = %.10lf\n", f.getX().calcMax());
        printf("Min y (NEW) = %.10lf\n", f.getY().calcMin());
        printf("Max y (NEW) = %.10lf\n", f.getY().calcMax());
        
        printf("Mean x (NEW) = %.10lf\n", f.getX().calcMean());
        printf("Mean y (NEW) = %.10lf\n", f.getY().calcMean());
        printf("Var x (NEW) = %.10lf\n", f.getX().calcVar());
        printf("Var y (NEW) = %.10lf\n", f.getY().calcVar());
        printf("StdDev x (NEW) = %.10lf\n", f.getX().calcStdDev());
        printf("StdDev y (NEW) = %.10lf\n", f.getY().calcStdDev());
        printf("Covariance (NEW) = %.10lf\n", f.calcCov());
        printf("Correlation (NEW) = %.10lf\n", f.calcCorrel());
    }

    
    {
        printf("\n\nPositive Correlation and Low Covariance Data Set\n");
        double xarr[] = { 0, 2, 4, 6, 8 };
        double yarr[] = { 6, 13, 15, 16, 20 };

        calc.SetValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]));

        printf("Mean x = %.10lf\n", calc.x.CalculateMean());
        printf("Mean y = %.10lf\n", calc.y.CalculateMean());
        printf("Var x = %.10lf\n", calc.x.CalculateVariane());
        printf("Var y = %.10lf\n", calc.y.CalculateVariane());
        printf("StdDev x = %.10lf\n", calc.x.Calculate_StandardDeviation());
        printf("StdDev y = %.10lf\n", calc.y.Calculate_StandardDeviation());
        printf("Covariance = %.10lf\n", calc.Calculate_Covariance());
        printf("Correlation = %.10lf\n", calc.Calculate_Correlation());
        
        printf("\n=====================\n");
        
        FinCalc f;
        setValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]), f);
        
        printf("Min x (NEW) = %.10lf\n", f.getX().calcMin());
        printf("Max x (NEW) = %.10lf\n", f.getX().calcMax());
        printf("Min y (NEW) = %.10lf\n", f.getY().calcMin());
        printf("Max y (NEW) = %.10lf\n", f.getY().calcMax());
        
        printf("Mean x (NEW) = %.10lf\n", f.getX().calcMean());
        printf("Mean y (NEW) = %.10lf\n", f.getY().calcMean());
        printf("Var x (NEW) = %.10lf\n", f.getX().calcVar());
        printf("Var y (NEW) = %.10lf\n", f.getY().calcVar());
        printf("StdDev x (NEW) = %.10lf\n", f.getX().calcStdDev());
        printf("StdDev y (NEW) = %.10lf\n", f.getY().calcStdDev());
        printf("Covariance (NEW) = %.10lf\n", f.calcCov());
        printf("Correlation (NEW) = %.10lf\n", f.calcCorrel());

    }

    {
        printf("\n\nNegative Correlation and Low Covariance Data Set\n");

        double xarr[] = { 8, 6, 4, 2, 0 };
        double yarr[] = { 6, 13, 15, 16, 20 };

        calc.SetValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]));

        printf("Mean x = %.10lf\n", calc.x.CalculateMean());
        printf("Mean y = %.10lf\n", calc.y.CalculateMean());
        printf("Var x = %.10lf\n", calc.x.CalculateVariane());
        printf("Var y = %.10lf\n", calc.y.CalculateVariane());
        printf("StdDev x = %.10lf\n", calc.x.Calculate_StandardDeviation());
        printf("StdDev y = %.10lf\n", calc.y.Calculate_StandardDeviation());
        printf("Covariance = %.10lf\n", calc.Calculate_Covariance());
        printf("Correlation = %.10lf\n", calc.Calculate_Correlation());
        
        printf("\n=====================\n");
        
        FinCalc f;
        setValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]), f);
        
        printf("Min x (NEW) = %.10lf\n", f.getX().calcMin());
        printf("Max x (NEW) = %.10lf\n", f.getX().calcMax());
        printf("Min y (NEW) = %.10lf\n", f.getY().calcMin());
        printf("Max y (NEW) = %.10lf\n", f.getY().calcMax());
        
        printf("Mean x (NEW) = %.10lf\n", f.getX().calcMean());
        printf("Mean y (NEW) = %.10lf\n", f.getY().calcMean());
        printf("Var x (NEW) = %.10lf\n", f.getX().calcVar());
        printf("Var y (NEW) = %.10lf\n", f.getY().calcVar());
        printf("StdDev x (NEW) = %.10lf\n", f.getX().calcStdDev());
        printf("StdDev y (NEW) = %.10lf\n", f.getY().calcStdDev());
        printf("Covariance (NEW) = %.10lf\n", f.calcCov());
        printf("Correlation (NEW) = %.10lf\n", f.calcCorrel());
    }

    {
        printf("\n\nPositive Correlation and High Covariance Data Set\n");
        double xarr[] = { 8, 6, 4, 2, 0 };
        double yarr[] = { 1006, 513, 315, 216, 120 };

        calc.SetValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]));

        printf("Mean x = %.10lf\n", calc.x.CalculateMean());
        printf("Mean y = %.10lf\n", calc.y.CalculateMean());
        printf("Var x = %.10lf\n", calc.x.CalculateVariane());
        printf("Var y = %.10lf\n", calc.y.CalculateVariane());
        printf("StdDev x = %.10lf\n", calc.x.Calculate_StandardDeviation());
        printf("StdDev y = %.10lf\n", calc.y.Calculate_StandardDeviation());
        printf("Covariance = %.10lf\n", calc.Calculate_Covariance());
        printf("Correlation = %.10lf\n", calc.Calculate_Correlation());
        
        printf("\n=====================\n");
        
        FinCalc f;
        setValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]), f);
        
        printf("Min x (NEW) = %.10lf\n", f.getX().calcMin());
        printf("Max x (NEW) = %.10lf\n", f.getX().calcMax());
        printf("Min y (NEW) = %.10lf\n", f.getY().calcMin());
        printf("Max y (NEW) = %.10lf\n", f.getY().calcMax());
        
        printf("Mean x (NEW) = %.10lf\n", f.getX().calcMean());
        printf("Mean y (NEW) = %.10lf\n", f.getY().calcMean());
        printf("Var x (NEW) = %.10lf\n", f.getX().calcVar());
        printf("Var y (NEW) = %.10lf\n", f.getY().calcVar());
        printf("StdDev x (NEW) = %.10lf\n", f.getX().calcStdDev());
        printf("StdDev y (NEW) = %.10lf\n", f.getY().calcStdDev());
        printf("Covariance (NEW) = %.10lf\n", f.calcCov());
        printf("Correlation (NEW) = %.10lf\n", f.calcCorrel());
    }

    {
        printf("\n\nNegative Correlation and High Covariance Data Set\n");
        double xarr[] = { 8, 6, 4, 2, 0 };
        double yarr[] = { 120, 216, 315, 513, 1006 };

        calc.SetValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]));

        printf("Mean x = %.10lf\n", calc.x.CalculateMean());
        printf("Mean y = %.10lf\n", calc.y.CalculateMean());
        printf("Var x = %.10lf\n", calc.x.CalculateVariane());
        printf("Var y = %.10lf\n", calc.y.CalculateVariane());
        printf("StdDev x = %.10lf\n", calc.x.Calculate_StandardDeviation());
        printf("StdDev y = %.10lf\n", calc.y.Calculate_StandardDeviation());
        printf("Covariance = %.10lf\n", calc.Calculate_Covariance());
        printf("Correlation = %.10lf\n", calc.Calculate_Correlation());
        
        printf("\n=====================\n");
        
        FinCalc f;
        setValues(xarr,yarr,sizeof(xarr) / sizeof(xarr[0]), f);
        
        printf("Min x (NEW) = %.10lf\n", f.getX().calcMin());
        printf("Max x (NEW) = %.10lf\n", f.getX().calcMax());
        printf("Min y (NEW) = %.10lf\n", f.getY().calcMin());
        printf("Max y (NEW) = %.10lf\n", f.getY().calcMax());
        
        printf("Mean x (NEW) = %.10lf\n", f.getX().calcMean());
        printf("Mean y (NEW) = %.10lf\n", f.getY().calcMean());
        printf("Var x (NEW) = %.10lf\n", f.getX().calcVar());
        printf("Var y (NEW) = %.10lf\n", f.getY().calcVar());
        printf("StdDev x (NEW) = %.10lf\n", f.getX().calcStdDev());
        printf("StdDev y (NEW) = %.10lf\n", f.getY().calcStdDev());
        printf("Covariance (NEW) = %.10lf\n", f.calcCov());
        printf("Correlation (NEW) = %.10lf\n", f.calcCorrel());
    }
    
    {
        printf("\n\nTesting stats buckets\n");
        printf("\n=====================\n");
        
        tw::common::StatsManager<> statsManager;
        
        std::string name1 = "testName1";
        std::string name2 = "testName2";
        
        size_t i1 = statsManager.registerNewStat(name1);
        size_t i2 = statsManager.registerNewStat(name2);
        
        for ( size_t i = 0; i < 10000; ++i ) {
            if ( i < 7500 ) {
                statsManager.addValue(i1, i%3);
                statsManager.addValue(i2, i%3+10);
            } else {
                statsManager.addValue(i1, i%5+15);
                statsManager.addValue(i2, i%3+105);
            }
        }
        
        printf("Stats: %s", statsManager.toString().c_str());
        printf("\n=====================\n");
        
        statsManager.clearStats();
        for ( size_t i = 0; i < 1000; ++i ) {
            if ( i < 750 ) {
                statsManager.addValue(i1, i%2+5);
                statsManager.addValue(i2, i%2+10);
            } else {
                statsManager.addValue(i1, i%5+45);
                statsManager.addValue(i2, i%3+205);
            }
        }
        
        printf("Stats: %s", statsManager.toString().c_str());
        printf("\n=====================\n");
        
        statsManager.clearStats();
        for ( size_t i = 0; i < 1000; ++i ) {
            statsManager.addValue(i1, i%2+5);
            statsManager.addValue(i2, i%2+10);
        }
        
        printf("Stats: %s\n", statsManager.toString().c_str());
        
    }

    return 0;
}
