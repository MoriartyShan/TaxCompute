#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
//#include <bits/shared_ptr.h>
#include <memory>


template <typename T>
std::string GetVector(const std::vector<T>& vec) {
  std::stringstream ss;
  ss.precision(8);
  for (int i = 0; i < vec.size(); i++) {
    ss << (std::to_string(vec[i]) + ",");
  }
  return ss.str();
}



class SocialSecurity{
public:
  enum artical {
    Endowment = 0,
    Medical = 1,
    Maternity = 2,
    Unemployment = 3,
    Injury = 4,
    Accumulation = 5,
    Tnumber = 6
  };
  double base[Tnumber] = {
    3613, //endowment_
    5557, //medical_
    5557, //maternity_
    3613, //unemployment_
    4713, //injury_
    3000, //accumulation_
  };

  double rate[2][Tnumber] = {
    {0.16, 0.1, 0.008, 0.008, 0.001, 0.12}, //company
    {0.08, 0.02, 0, 0, 0, 0.12} //yourself
  };

  std::shared_ptr<std::vector<std::vector<double>>> security_;

  SocialSecurity(){
    Compute();
  }

  SocialSecurity(const double _base){
    if (base != 0) {
      for (int i = 0; i < Tnumber; i++) {
        base[i] = _base;
      }
    }
    Compute();
  }

  void Compute() {
    security_ =
      std::make_shared<std::vector<std::vector<double>>>(
        2, std::vector<double>(Tnumber + 1, 0));

    auto &&res = *security_;
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < Tnumber; j++) {
        res[i][j] = rate[i][j] * base[j];
        if (Medical == j && i == 1) {
          res[i][j] += 3;
        }
        res[i][Tnumber] += res[i][j];
      }
    }
    return;
  }

  const std::shared_ptr<std::vector<std::vector<double>>> GetResult(){
    return security_;
  }
};

#define MONTH_NUM 12
const double start_point = 60000;
const std::vector<double> base_floor = {0, 36000, 144000, 300000, 420000, 660000, 960000};
const std::vector<double> rate_floor = {0.03, 0.1, 0.2, 0.25, 0.3, 0.35, 0.45};
class PersonSalaryInformation {
public:
  std::vector<double> each_month_reductions;
  std::vector<double> each_month_salaries;
  double annual_salary_ = 0;
  std::shared_ptr<SocialSecurity> security_ptr_;
  double annual_tax_;
  std::vector<double> month_tax;

  PersonSalaryInformation() {
    security_ptr_ = std::make_shared<SocialSecurity>();
    UpdateReductions(0);
    UpdateSalaries(0);
  }

  PersonSalaryInformation(const double salary = 0, const double reduction = 0) {
    security_ptr_ = std::make_shared<SocialSecurity>(salary);
    UpdateReductions(reduction);
    UpdateSalaries(salary);
  }

  PersonSalaryInformation(
      const double salary, const double base, const double reduction) {
    security_ptr_ = std::make_shared<SocialSecurity>(base);
    UpdateReductions(reduction);
    UpdateSalaries(salary);
  }

  void SetBaselineLowest() {
    security_ptr_ = std::make_shared<SocialSecurity>();
  }

  void UpdateReductions(const double reduction) {
    each_month_reductions.resize(MONTH_NUM, reduction);
  }
  void UpdateReductions(const std::vector<double>& reductions) {
    CHECK(reductions.size() == MONTH_NUM)
        << "reductions size must be 12 (currently is " << reductions.size() << ")";
    each_month_reductions = reductions;
  }

  void UpdateSalaries(const double salary) {
    each_month_salaries.resize(MONTH_NUM, salary);
  }

  void UpdateSalaries(const std::vector<double> &_salaries) {
    CHECK(_salaries.size() == MONTH_NUM)
        << "reductions size must be 12 (currently is " << _salaries.size() << ")";
    each_month_salaries = _salaries;
  }

  double ComputeFloorTaxEachMonth(const int month_id) {
    const int floot_num = base_floor.size();
    auto res = security_ptr_->GetResult();
    double taxed_salary = 0;
    for (int i = 0; i < (month_id + 1); i++) {
      taxed_salary += (each_month_salaries[i] - (*res)[1][SocialSecurity::Tnumber] - each_month_reductions[i]);
    }
    taxed_salary -= (start_point / MONTH_NUM * (month_id + 1));
    std::vector<double> taxs(floot_num + 1, 0);
    for (int i = 1; i < floot_num; i++) {
      if (taxed_salary <= base_floor[i]) {
        taxs[i - 1] = (taxed_salary - base_floor[i - 1]) * rate_floor[i - 1];
        taxs[floot_num] += taxs[i - 1];
        break;
      } else {
        taxs[i - 1] = (base_floor[i] - base_floor[i - 1]) * rate_floor[i - 1];
        taxs[floot_num] += taxs[i - 1];
      }

    }
    if (taxed_salary > base_floor[floot_num - 1]) {
      taxs[floot_num - 1] =
        (taxed_salary - base_floor[floot_num - 1]) * rate_floor[floot_num - 1];
      taxs[floot_num] += taxs[floot_num - 1];
    }
    return taxs[floot_num];
  }

  double inline ComputeAnnualTax() {
    annual_tax_ = month_tax[MONTH_NUM];
    return annual_tax_;
  }

  void ComputeMonthTax(){
    month_tax.resize(MONTH_NUM + 1);
    for (int i = 0; i < MONTH_NUM; i++) {
      month_tax[i] = ComputeFloorTaxEachMonth(i);
    }
    month_tax[MONTH_NUM] = month_tax[MONTH_NUM - 1];

    for (int i = (MONTH_NUM - 1); i > 0; i--) {
      month_tax[i] = month_tax[i] - month_tax[i - 1];
    }
  }

  void Compute() {
    ComputeMonthTax();
    ComputeAnnualTax();
  }

  double AnnualTax() const {return annual_tax_;}
  const std::vector<double> & MonthTaxs() const {return month_tax;}

};



int main(int argc, char* argv[]) {
  google::SetVersionString("1.0.0");
  google::SetUsageMessage(std::string(argv[0]) + " [OPTION]");
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]); // option --[also]logtostderr

  PersonSalaryInformation person(26600, 2500);
  person.SetBaselineLowest();
  person.Compute();
  auto month = person.month_tax;
  LOG(ERROR) << "Result\n" << person.AnnualTax();

  LOG(ERROR) << "Each month:" << GetVector(month);



  std::cout << "Hello, World!" << std::endl;
  return 0;
}