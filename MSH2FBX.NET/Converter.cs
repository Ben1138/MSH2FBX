using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSH2FBX
{
    // Bit Flags
    public enum EChunkFilter : byte
    {
        None = 0,
        Materials = 1,
        Models = 2,
        Animations = 4,
        Weights = 8
    }

    public class Converter
    {
        IntPtr Instance;
        APIWrapper.LogCallback callback = new APIWrapper.LogCallback(OnLog);
        public static Action<string> OnLog;

        public EChunkFilter ChunkFilter
        {
            get { return APIWrapper.Converter_Get_ChunkFilter(Instance); }
            set { APIWrapper.Converter_Set_ChunkFilter(Instance, value); }
        }

        public string OverrideAnimName
        {
            get { return APIWrapper.Converter_Get_OverrideAnimName(Instance); }
            set { APIWrapper.Converter_Set_OverrideAnimName(Instance, value); }
        }

        public bool EmptyMeshes
        {
            get { return APIWrapper.Converter_Get_EmptyMeshes(Instance); }
            set { APIWrapper.Converter_Set_EmptyMeshes(Instance, value); }
        }

        public string BaseposeMSH
        {
            get { return APIWrapper.Converter_Get_BaseposeMSH(Instance); }
            set { APIWrapper.Converter_Set_BaseposeMSH(Instance, value); }
        }


        public Converter()
        {
            Instance = APIWrapper.Converter_Create();
            APIWrapper.Converter_SetLogCallback(Instance, callback);
        }

        public Converter(string fbxFileName)
        {
            Instance = APIWrapper.Converter_Create_Start(fbxFileName);
            APIWrapper.Converter_SetLogCallback(Instance, callback);
        }

        ~Converter()
        {
            APIWrapper.Converter_Destroy(Instance);
        }


        public bool Start(string fbxFileName)
        {
            return APIWrapper.Converter_Start(Instance, fbxFileName);
        }

        public bool AddMSH(string mshFileName)
        {
            return APIWrapper.Converter_AddMSH(Instance, mshFileName);
        }

        public bool SaveFBX()
        {
            return APIWrapper.Converter_SaveFBX(Instance);
        }

        public bool ClearFBXScene()
        {
            return APIWrapper.Converter_ClearFBXScene(Instance);
        }

        public void Close()
        {
            APIWrapper.Converter_Close(Instance);
        }
    }
}