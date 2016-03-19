package utilityMatrix.recoEval.profiles;

public interface ProfilesHolder<T> {

   public T getProfile(int user);

   public int getNbUsers();

   public int[] getUserIds();

   public void dumpToFile(String file);

   //Added by Diego
   public boolean addIfAbsent(int id, T u);

   public T removeIfPresent(int id);

   public boolean addIfAbsentOrEmpty(int id, T u);


}
