using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneChanger : MonoBehaviour
{

	public int ProblemGenerator;
	public int MainScene;
	public int MainMenu;

	public void MoveToProblemGenerator()
	{
		SceneManager.LoadScene(ProblemGenerator);
	}

	public void MoveToMainScene()
	{
		SceneManager.LoadScene(MainScene);
	}

	public void MoveToMainMenu()
	{
		SceneManager.LoadScene(MainMenu);
	}
}
