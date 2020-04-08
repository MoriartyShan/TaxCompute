#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
//#include <bits/shared_ptr.h>
#include <memory>
#include <fstream>
#include <set>
#include<numeric>

template <typename T>
std::string GetVector(const std::vector<T>& vec) {
  std::stringstream ss;
  ss.precision(8);
  for (int i = 0; i < vec.size(); i++) {
    ss << (std::to_string(vec[i]) + ",");
  }
  return ss.str();
}

struct StageValue {
  //from start to end, its value
  int start = 0;
  int end = 0;
  double value = 0;
  StageValue(){}
  StageValue(const int start_, const int end_, const double value_):
    start(start_), end(end_), value(value_){}
};

class PersonalReader {
private:
  std::vector<StageValue> salaries_;
  std::vector<StageValue> social_security_bases_;
  std::vector<StageValue> tax_reductions_;
  std::vector<std::set<int>> test = {
    std::set<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}), //salary
    std::set<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}), //social base
    std::set<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}) //tax_reductions_
  };
public:
  PersonalReader(const std::string &path){
    std::ifstream file(path);

    const std::string s[3] = {"salarys", "social_security", "reduction"};
    CHECK(file.is_open()) << "file: " << path << " not exist";
    char bar;
    std::string line;
    int start, end;
    double value;
    std::vector<StageValue> *cur_ptr = nullptr;
    std::set<int> *test_prt = nullptr;
    while(!file.eof()) {
      std::getline(file, line);
      if (line.empty()) {
        break;
      }
      if (line.front() == '#') {
        continue;
      }

      if (line.front() == 's' || line.front() == 'r') {
        if (line == s[0]) {
          cur_ptr = &salaries_;
          test_prt = &test[0];
        } else if (line == s[1]) {
          cur_ptr = &social_security_bases_;
          test_prt = &test[1];
        } else if (line == s[2]) {
          cur_ptr = &tax_reductions_;
          test_prt = &test[2];
        } else {
          LOG(FATAL) << "Wrong input mark(" << line << ")";
        }
        continue;
      }

      std::stringstream ss(line);

      ss >> start;
      ss >> bar;
      if (bar == '-') {
        ss >> end;
        ss >> bar;
      } else if (bar == ',') {
        end = start;
      } else {
        LOG(FATAL) << "Invalid split (" << bar << ")";
      }
      ss >> value;
      CHECK(start <= end && start >= 1) << "Wrong input (" << start << "," << end << ")";
      for (int i = start; i <= end; i++) {
        CHECK(test_prt->count(i) > 0) << "something appeared more than once(" << i << ")";
        test_prt->erase(i);
      }
      cur_ptr->emplace_back(start, end, value);
    }
    file.close();
    CHECK(isValid()) << "Not enough input month";
  }

  bool isValid() const {
    for (int i = 0; i < test.size(); i++) {
      if (test[i].size() > 0) return false;
    }
    return true;
  }

  const std::vector<StageValue> &social_bases() const {
    return social_security_bases_;
  }

  const std::vector<StageValue> &salaries() const {
    return salaries_;
  }

  const std::vector<StageValue> &reductions() const {
    return tax_reductions_;
  }
};

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
  std::vector<double> base = {
    3613, //endowment_
    5557, //medical_
    5557, //maternity_
    3613, //unemployment_
    4713, //injury_
    3000, //accumulation_
  };

  std::vector<double> rate[2] = {
    {0.16, 0.1, 0.008, 0.008, 0.001, 0.12}, //company
    {0.08, 0.02, 0, 0, 0, 0.12} //yourself
  };

  std::shared_ptr<std::vector<std::vector<double>>> security_;

  SocialSecurity(){
    Compute();
  }

  SocialSecurity(const double _base){

    for (int i = 0; i < Tnumber; i++) {
      if (base[i] < _base) {
        base[i] = _base;
      }
    }

    Compute();
  }
//  SocialSecurity(const std::vector<StageValue>& bases){
//    CHECK(bases.size() == 12);
//    for (int i = 0; i < bases.size(); i++) {
//      for (int j = bases[i].start; j <= bases[i].end; j++) {
//        base[j] = bases[i].value;
//      }
//    }
//    Compute();
//  }

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

  const std::shared_ptr<std::vector<std::vector<double>>> GetResult() const{
    return security_;
  }
  double PersionalSecurityPayment() const {return (*security_)[1][Tnumber];}
};

#define MONTH_NUM 12
const double start_point = 60000;
const std::vector<double> base_floor = {0, 36000, 144000, 300000, 420000, 660000, 960000};
const std::vector<double> rate_floor = {0.03, 0.1, 0.2, 0.25, 0.3, 0.35, 0.45};

class PersonSalaryInformation {
public:
  std::vector<double> each_month_reductions;
  std::vector<double> each_month_salaries;
  std::vector<std::shared_ptr<SocialSecurity>> securities_;
  double annual_tax_, annual_security_output_;
  double before_tax_annual_salary_, after_tax_annual_salary_;
  std::vector<double> month_tax;

  PersonSalaryInformation() {
    UpdateSocialSecurity(0);
    UpdateReductions(0);
    UpdateSalaries(0);
    Compute();
  }

  PersonSalaryInformation(const double salary = 0, const double reduction = 0) {
    UpdateSocialSecurity(salary);
    UpdateReductions(reduction);
    UpdateSalaries(salary);
    Compute();
  }

  PersonSalaryInformation(
      const double salary, const double base, const double reduction) {
    UpdateSocialSecurity(base);
    UpdateReductions(reduction);
    UpdateSalaries(salary);
    Compute();
  }

  PersonSalaryInformation(
      const PersonalReader& personal) {
    CHECK(personal.isValid()) << "persional info is not valid";
    UpdateSocialSecurity(personal.social_bases());
    UpdateReductions(personal.reductions());
    UpdateSalaries(personal.salaries());
    Compute();
  }
  void SetBaselineLowest() {
    UpdateSocialSecurity(0);
  }

  void UpdateSocialSecurity(const double base) {
    securities_.resize(MONTH_NUM, std::make_shared<SocialSecurity>(base));
    annual_security_output_ = securities_[0]->PersionalSecurityPayment() * MONTH_NUM;
  }

  void UpdateSocialSecurity(const std::vector<StageValue>& persional_bases) {
    securities_.resize(MONTH_NUM);
    annual_security_output_ = 0;
    for (int i = 0; i < persional_bases.size(); i++) {
      for (int j = (persional_bases[i].start - 1); j <= (persional_bases[i].end - 1); j++) {
        securities_[j] = std::make_shared<SocialSecurity>(persional_bases[i].value);
        annual_security_output_ += securities_[j]->PersionalSecurityPayment();
      }
    }
  }

  void UpdateReductions(const double reduction) {
    each_month_reductions.resize(MONTH_NUM, reduction);
  }

  void UpdateReductions(const std::vector<double>& reductions) {
    CHECK(reductions.size() == MONTH_NUM)
        << "reductions size must be 12 (currently is " << reductions.size() << ")";
    each_month_reductions = reductions;
  }

  void UpdateReductions(const std::vector<StageValue>& reductions) {
    each_month_reductions.resize(MONTH_NUM, 0);
    for (int i = 0; i < reductions.size(); i++) {
      for (int j = reductions[i].start - 1; j <= (reductions[i].end - 1); j++) {
        each_month_reductions[j] = reductions[i].value;
      }
    }

  }

  void UpdateSalaries(const double salary) {
    before_tax_annual_salary_ = salary * MONTH_NUM;
    LOG(ERROR) << "before = " << before_tax_annual_salary_;
    each_month_salaries.resize(MONTH_NUM, salary);
  }

  void UpdateSalaries(const std::vector<double> &_salaries) {
    CHECK(_salaries.size() == MONTH_NUM)
        << "reductions size must be 12 (currently is " << _salaries.size() << ")";
    each_month_salaries = _salaries;
    before_tax_annual_salary_ =
        std::accumulate(each_month_salaries.begin(), each_month_salaries.end(), 0.0);
    LOG(ERROR) << "before = " << before_tax_annual_salary_;
  }

  void UpdateSalaries(const std::vector<StageValue>& salaries) {
    each_month_salaries.resize(MONTH_NUM, 0);
    before_tax_annual_salary_ = 0;
    for (int i = 0; i < salaries.size(); i++) {
      for (int j = (salaries[i].start - 1); j <= (salaries[i].end - 1); j++) {
        each_month_salaries[j] = salaries[i].value;
        before_tax_annual_salary_ += each_month_salaries[j];
      }
    }
  }

  std::vector<const std::vector<std::vector<double>>*>
      GetSocialSecurityResult() {
    std::vector<const std::vector<std::vector<double>>*> result;
    CHECK(securities_.size() == 12);
    for (int i = 0; i < MONTH_NUM; i++) {
      result.emplace_back(securities_[i]->GetResult().get());
    }
    return result;
  }

  double ComputeFloorTaxEachMonth(const int month_id) {
    const int floot_num = base_floor.size();
    auto social_security = GetSocialSecurityResult();
    double taxed_salary = 0;
    for (int i = 0; i < (month_id + 1); i++) {
      taxed_salary += (each_month_salaries[i] - (*social_security[i])[1][SocialSecurity::Tnumber] - each_month_reductions[i]);
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
    after_tax_annual_salary_ = before_tax_annual_salary_ - AnnualTax() - annual_security_output_;
  }

  double AnnualTax() const {return annual_tax_;}
  const std::vector<double> & MonthTaxs() const {return month_tax;}

  double BeforeTaxAnualSalary() const {
    return before_tax_annual_salary_;
  }

  double AfterTaxAnualSalary() const {
    return after_tax_annual_salary_;
  }

};



int main(int argc, char* argv[]) {
  google::SetVersionString("1.0.0");
  google::SetUsageMessage(std::string(argv[0]) + " [OPTION]");
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]); // option --[also]logtostderr

  PersonalReader informations("../salaries.txt");

  PersonSalaryInformation person(informations);
//  person.SetBaselineLowest();
//  person.Compute();
  auto month = person.MonthTaxs();
  LOG(ERROR) << "Result\n" << person.AnnualTax();
  LOG(ERROR) << "Each month:" << GetVector(month);
  LOG(ERROR) << "BeforeTaxAnualSalary = " << person.BeforeTaxAnualSalary() << " AfterTaxAnualSalary = " << person.AfterTaxAnualSalary();
  return 0;
}