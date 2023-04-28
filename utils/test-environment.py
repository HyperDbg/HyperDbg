import shutil
import os

def copy_dir(from_dir, to_dir) :
    files_in_src = os.listdir(from_dir)
    
    for file_name in files_in_src:
        full_file_name = os.path.join(from_dir, file_name)
        if os.path.isfile(full_file_name):
            shutil.copy(full_file_name,  os.path.join(to_dir, file_name))
    

def main():

    #  
    # Create script engine test files
    #
    print("Creating script engine test environment...")
    
    from_dir = "..\\hyperdbg\\script-engine\\modules\\script-engine-test\\script-test-cases"
    to_dir1 = "..\\hyperdbg\\hyperdbg-cli\\script-test-cases"
    
    if os.path.exists(to_dir1) and os.path.isdir(to_dir1):
        shutil.rmtree(to_dir1)

    os.mkdir(to_dir1)
    
    copy_dir(from_dir, to_dir1)
    
    #
    # copy to build directories
    #
    to_dir2 = "..\\hyperdbg\\build\\bin\\debug"
    to_dir3 = "..\\hyperdbg\\build\\bin\\release"
    
    if os.path.exists(to_dir2) :
        to_dir2 = to_dir2 + "\\script-test-cases"
        
        if os.path.exists(to_dir2) and os.path.isdir(to_dir2):
            shutil.rmtree(to_dir2)
            
        os.mkdir(to_dir2)
        copy_dir(from_dir, to_dir2)    
        
    if os.path.exists(to_dir3) :
        to_dir3 = to_dir3 + "\\script-test-cases"
        
        if os.path.exists(to_dir3) and os.path.isdir(to_dir3):
            shutil.rmtree(to_dir3)
            
        os.mkdir(to_dir3)
        copy_dir(from_dir, to_dir3)

        
        
    if os.path.exists(to_dir2) :
        to_dir2 = to_dir2 + "\\script-test-cases"
    
    
    print("Testing environment is made successfully")


if __name__ == "__main__":
    main()