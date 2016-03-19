package evaluation.common;

import evaluation.workload.Workload;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 13/09/14
 */
public class ProteusTMDataSet {
   private final ProfilesHolder<UserProfile> workloadProfiles;
   private final int numConfigsPerWorkload;

   public ProteusTMDataSet(ProfilesHolder<UserProfile> workloadProfiles, int numConfigsPerWorkload) {
      this.workloadProfiles = workloadProfiles;
      this.numConfigsPerWorkload = numConfigsPerWorkload;
   }

   public int getNumConfigsPerWorkload() {
      return numConfigsPerWorkload;
   }

   public int getNumWorkloads() {
      return workloadProfiles.getNbUsers();
   }

   public ProfilesHolder<UserProfile> getWorkloadProfiles() {
      return workloadProfiles;
   }

   public void removeWorkload(Workload w) {
      final UserProfile u = w.getProfile();
      final int id = w.getId();
      final boolean removed = workloadProfiles.removeIfPresent(id) != null;
      if (!removed)
         throw new IllegalArgumentException("Workload " + w + " not present");
   }

   public void addWorkload(Workload w) {
      final UserProfile u = w.getProfile();
      final int id = w.getId();
      //TODO: the addIfAbsent method checks whether the user is there. In this scheme, the user IS indeed there
      //TODO but its profile is empty; so, I should check whether the corresponding profile is empty or not
      final boolean added = workloadProfiles.addIfAbsentOrEmpty(id, u);
      if (!added) {
         throw new IllegalArgumentException("Workload " + w + " not added: current workloads are " + workloadProfiles.toString());
      }
   }


}
