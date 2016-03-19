package utilityMatrix.recoEval.profiles;

import gnu.trove.iterator.TIntIterator;

public interface UserProfile {

   public int getNbItems();

   public TIntIterator getItems();

   public boolean contains(int item);

   public double getRating(int item);

   public double getNorm();

   //Added by Diego
   public boolean add(int item, double rating);

   public boolean remove(int item);

   public UserProfile copyUserProfile();

}
