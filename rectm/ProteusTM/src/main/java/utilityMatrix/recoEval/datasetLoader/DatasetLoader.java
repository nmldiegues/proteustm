package utilityMatrix.recoEval.datasetLoader;

import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

public interface DatasetLoader {

   public ProfilesHolder<UserProfile> getUserProfiles();

   public int getNbUsers();

   public int getNbDimensions();

   public String getName();

}
