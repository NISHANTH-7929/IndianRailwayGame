#pragma once

namespace Rail {
// Diagnostic launch policy only; never changes checkpoint formats or gameplay.
struct ReviewPolicy {
 bool isolated=false;
 bool resumeRequired=false;
 static ReviewPolicy FromFlags(bool development,bool review,bool egmore,bool resume){
  return {development&&review,development&&review&&egmore&&resume};
 }
 bool Fresh() const{return isolated&&!resumeRequired;}
 bool RecoveryBlocked(bool recovered,bool hasSlots) const{
  return !Fresh()&&!recovered&&(resumeRequired||hasSlots);
 }
};
}
