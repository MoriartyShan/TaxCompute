#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
//#include <bits/shared_ptr.h>
#include <memory>
#include <fstream>
#include <set>
#include<numeric>

DEFINE_string(infomation, "./salaries.txt", "file path to your salary file");

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
  std::vector<double> base = {
    3613, //endowment_
    5557, //medical_
    5557, //maternity_
    3613, //unemployment_
    4713, //injury_
    3000, //accumulation_
  };

  std::vector<std::vector<double>> rate = Shanghai_standard_rate;

  static std::vector<std::vector<double>> Beijing_standard_rate;
  static std::vector<std::vector<double>> Shanghai_standard_rate;
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

std::vector<std::vector<double>> SocialSecurity::Beijing_standard_rate = {
  {0.16, 0.1, 0.008, 0.008, 0.001, 0.12}, //company
  {0.08, 0.02, 0, 0.02, 0, 0.12} //yourself
  //endowment_,medical_,maternity_,unemployment_,injury_,accumulation_
  //养老，医疗，生育，失业，工伤，公积金
};
std::vector<std::vector<double>> SocialSecurity::Shanghai_standard_rate = {
  {0.21, 0.09, 0.01, 0.02, 0.005, 0.12}, //company
  {0.08, 0.02, 0, 0.01, 0, 0.12} //yourself
  //endowment_,medical_,maternity_,unemployment_,injury_,accumulation_
  //养老，医疗，生育，失业，工伤，公积金
};

template <typename T>
struct StageValue {
  //from start to end, its value
  int start = 0;
  int end = 0;
  T value;
  StageValue(){}
  StageValue(const int start_, const int end_, const T value_):
    start(start_), end(end_), value(value_){}
};
using StageValued = StageValue<double>;
using StageValueVectord = StageValue<std::vector<double>>;
using VectorStageValued = std::vector<StageValued>;
class PersonalReader {
private:
  enum Artical {
    MonthSalary = 0,
    SocialSecurityBase = 1,
    TaxReduction = 2,
    SocialSecirityRateCompany = 3,
    SocialSecirityRatePersional = 4,
    TotalArticals
  };
  std::vector<VectorStageValued> double_inputs_;/*MonthSalary, SocialSecurityBase, TaxReduction*/
  std::vector<std::vector<StageValueVectord>> ssrates;/*SocialSecirityRateCompany, SocialSecirityRatePersional*/
  std::vector<std::set<int>> record;
public:
  PersonalReader(const std::string &path){
    std::ifstream file(path);
    record.resize(TotalArticals, std::set<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}));
    double_inputs_.resize(SocialSecirityRateCompany);
    ssrates.resize((TotalArticals - SocialSecirityRateCompany));
    const std::string artical_mode = "Artical:";
    CHECK(file.is_open()) << "file: " << path << " not exist";
    char bar;
    std::string line;
    int start, end;
    double value;
    std::vector<double> values(SocialSecurity::artical::Tnumber);
    int article_id = -1;
    while(!file.eof()) {
      std::getline(file, line);
      if (line.empty()) {
        continue;
      }
      if (line.front() == '#') {
        continue;
      }

      if (line.front() >= '0' && line.front() <= '9') {
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
        CHECK(start <= end && start >= 1) << "Wrong input (" << start << "," << end << ")";
        if (article_id >=MonthSalary && article_id <= TaxReduction) {
          ss >> value;
          ss >> bar;
          double_inputs_[article_id].emplace_back(start, end, value);
        } else if (article_id >= SocialSecirityRateCompany && article_id <= SocialSecirityRatePersional){
          for (int i = 0; i < SocialSecurity::artical::Tnumber; i++) {
            ss >> values[i];
            ss >> bar;
          }
          ssrates[(article_id - SocialSecirityRateCompany)].emplace_back(start, end, values);
        } else {
          LOG(FATAL) << "You must appoint a artical in the first(#not count) line";
        }
        //erase articals record
        for (int i = start; i <= end; i++) {
          CHECK(record[article_id].count(i) > 0) << "something appeared more than once(" << i << ")";
          record[article_id].erase(i);
        }

      } else {
        CHECK(line.substr(0, artical_mode.length()) == artical_mode) << "Wrong input line:" << line;
        std::string left = line.substr(artical_mode.length());

        std::stringstream ss(left);
        ss >> article_id;
        CHECK(article_id >= MonthSalary && article_id <= SocialSecirityRatePersional)
            << "Wrong artical id" << article_id;
      }

    }
    file.close();
    CHECK(isValid()) << "Not enough input month";
  }

  bool isValid() const {
    for (int i = 0; i < record.size(); i++) {
      if (record[i].size() > 0) return false;
    }
    return true;
  }

  const VectorStageValued &social_bases() const {
    return double_inputs_[Artical::SocialSecurityBase];
  }

  const VectorStageValued &salaries() const {
    return double_inputs_[Artical::MonthSalary];
  }

  const VectorStageValued &reductions() const {
    return double_inputs_[Artical::TaxReduction];
  }

  const std::vector<std::vector<StageValueVectord>>&
      security_rates() const {
    return ssrates;
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
    UpdateSocialSecurity(personal);
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

  void UpdateSocialSecurity(const PersonalReader &personal) {
    securities_.resize(MONTH_NUM);
    annual_security_output_ = 0;
    //security base
    {
      auto security_bases = personal.social_bases();
      for (int i = 0; i < security_bases.size(); i++) {
        for (int j = (security_bases[i].start - 1); j <= (security_bases[i].end - 1); j++) {
          securities_[j] = std::make_shared<SocialSecurity>(security_bases[i].value);

        }
      }
    }
    auto rates = personal.security_rates();
    for (int i = 0; i < rates[0].size(); i++) {
      for (int j = (rates[0][i].start - 1); j <= (rates[0][i].end - 1); j++) {
        securities_[j]->rate[0] = (rates[0][i].value);
      }
    }

    for (int i = 0; i < rates[1].size(); i++) {
      for (int j = (rates[1][i].start - 1); j <= (rates[1][i].end - 1); j++) {
        securities_[j]->rate[1] = (rates[1][i].value);
      }
    }

    for (int i = 0; i < securities_.size(); i++) {
      securities_[i]->Compute();
      annual_security_output_ += securities_[i]->PersionalSecurityPayment();
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

  void UpdateReductions(const std::vector<StageValued>& reductions) {
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

  void UpdateSalaries(const std::vector<StageValued>& salaries) {
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

  PersonalReader informations(FLAGS_infomation);

  PersonSalaryInformation person(informations);

  auto month = person.MonthTaxs();
  LOG(ERROR) << "Result\n" << person.AnnualTax();
  LOG(ERROR) << "Each month:" << GetVector(month);
  LOG(ERROR) << "BeforeTaxAnualSalary = " << person.BeforeTaxAnualSalary() << " AfterTaxAnualSalary = " << person.AfterTaxAnualSalary();
  return 0;
}