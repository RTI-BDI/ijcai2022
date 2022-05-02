using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GridManager : MonoBehaviour
{
    [SerializeField]
    private int rows;
    [SerializeField]
    private int cols;
    [SerializeField]
    private float tileSize = 1;

    // Start is called before the first frame update
    void Start()
    {
       
    }

    public void GenerateGrid()
    {
        GameObject referenceTile = (GameObject)Instantiate(Resources.Load("GrassTile"));
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                GameObject tile = (GameObject)Instantiate(referenceTile, transform);

                float posX = col * tileSize + (tileSize - 1)/2;
                float posY = row * tileSize + (tileSize - 1)/2;

                tile.transform.position = new Vector2(posX, posY);
            }
        }

        Destroy(referenceTile);

        float gridWidth = cols * tileSize;
        float gridHeight = rows * tileSize;
        transform.position = new Vector2((-gridWidth / 2f) + (tileSize / 2f), (-gridHeight / 2f) + (tileSize / 2f));
    }

    // Update is called once per frame
    void Update()
    {
        
    }

	public void SetGridSize(int size)
	{
		this.rows = size;
		this.cols = size;
	}

    public float GetTileSize()
    {
        return this.tileSize;
    }

    public Vector2 GetPostion()
    {
        return transform.position;
    }
}
